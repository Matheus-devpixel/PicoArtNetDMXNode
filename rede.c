/**
 * ============================================================================
 * PROJETO: PICO ARTNET DMX NODE (RP2040)
 * ARQUIVO: rede.c
 * AUTOR: Matheus Dev (@Matheus-devpixel)
 * DESCRIÇÃO: Subsistema completo de comunicação em rede Ethernet over USB (RNDIS / CDC-ECM),
 *            pilha TCP/IP lwIP, receptor e processador de protocolo Art-Net v4
 *            (Porta UDP 6454), servidor DHCP embarcado, servidor DNS captive portal,
 *            servidor Web HTTP (Porta 80) com API REST, streaming DMX binário e SSE.
 *
 * ARQUITETURA DA CAMADA DE REDE:
 *   1. Driver USB Ethernet TinyUSB:
 *      - Enumera como adaptador de rede virtual Microsoft RNDIS (Windows Plug & Play)
 *        ou CDC-ECM (Linux / macOS).
 *      - Fila circular de pbufs de 16 posições para absorver rajadas intensas de 60 FPS.
 *   2. Pilha lwIP (Lightweight IP):
 *      - Encaminha pacotes entre o driver de rede USB e os sockets UDP e TCP.
 *      - Roteamento e tratamento de pacotes Art-Net de broadcast global e local.
 *   3. Servidor DHCP Embutido (dhserver):
 *      - Entrega automaticamente configuração IP ao computador host sem necessidade
 *        de configuração manual de adaptador no Windows.
 *   4. Receptor Art-Net UDP (Porta 6454):
 *      - Processa OpPoll (descoberta de nós) e responde com OpPollReply contendo
 *        identidade, universos suportados e capacidades.
 *      - Processa OpDmx com desempacotamento de até 8 universos físicos simultâneos.
 *      - Processa OpSync para sincronização atômica livre de tearing entre saídas DMX.
 *   5. Servidor Web HTTP (Porta 80):
 *      - Dashboard de monitoramento e gestão em tempo real.
 *      - Painel de configuração inicial (Setup Wizard) no IP 10.10.0.1.
 *      - Monitor gráfico de canais DMX (/monitor) e Testador Manual (/tester).
 *      - API REST (/api/test_channel, /api/test_all, /api/theme, /salvar, /reset).
 *      - Streaming binário contínuo em 60 FPS de todos os 512 canais (/stream).
 * ============================================================================
 */

#include "rede.h"
#include "globais.h"
#include "storage.h"
#include "lang.h"
#include "dmx.h"
#include "ssd1306.h"
#include "hardware/watchdog.h"
#include "hardware/regs/m0plus.h"
#include "hardware/regs/addressmap.h"
#include "pico/time.h"
#include "pico/unique_id.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/ip.h"

#include "tusb.h"
#include "dhserver.h"
#include "dnserver.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "lwip/etharp.h"
#include "netif/ethernet.h"
#include "html_monitor.h"
#include "html_tester.h"
#include "img_pinouts.h"

static struct udp_pcb *s_artnet_pcb = NULL;
static struct tcp_pcb *s_http_pcb = NULL;
static char s_ip_str[32] = "0.0.0.0";
static bool s_connected = false;
static bool s_usb_mode = true;

// Modo de Teste Manual de Faders (Prioridade Total aos Faders / Suspende Art-Net)
volatile bool g_manual_test_active = false;
static uint32_t s_last_manual_test_time = 0;

// Telemetria em tempo real do protocolo Art-Net
volatile uint32_t g_artnet_packets_total = 0;
volatile uint32_t g_artnet_packets_sec = 0;
volatile uint32_t g_artnet_pps = 0;
volatile uint32_t g_artnet_fps = 0;
volatile uint16_t g_artnet_last_universe = 0;
volatile uint8_t  g_artnet_ch1 = 0;
volatile uint8_t  g_artnet_ch2 = 0;
volatile uint8_t  g_artnet_ch3 = 0;
volatile bool     g_artnet_active = false;
volatile uint32_t g_artnet_last_packet_time = 0;
volatile uint32_t g_artnet_lost_packets = 0;
volatile bool     g_artsync_active = false;
volatile uint8_t  g_artnet_active_universes_mask = 0;
static uint32_t   s_last_artsync_time = 0;
static uint8_t    s_last_sequence[MAX_UNIVERSES] = {0};
static bool       s_has_sequence[MAX_UNIVERSES] = {false};

/**
 * @brief Informa se o sistema está operando em modo de rede USB Ethernet.
 * @return true sempre (firmware dedicado USB Ethernet RNDIS / CDC-ECM).
 */
bool rede_is_usb_mode(void) {
    return s_usb_mode;
}

// ============================================================================
// PILHA USB ETHERNET (TINYUSB RNDIS / CDC-ECM + LWIP)
// ============================================================================
static struct netif s_netif_data;

// Fila circular de recepção para absorver rajadas atômicas (burst) de 8 universos simultâneos (60 FPS)
#define RECV_QUEUE_SIZE 16
static struct pbuf *s_recv_queue[RECV_QUEUE_SIZE];
static volatile uint8_t s_recv_head = 0;
static volatile uint8_t s_recv_tail = 0;

static inline bool recv_queue_is_full(void) {
    return ((uint8_t)(s_recv_head + 1) % RECV_QUEUE_SIZE) == s_recv_tail;
}

static inline bool recv_queue_is_empty(void) {
    return s_recv_head == s_recv_tail;
}

uint8_t tud_network_mac_address[6] = {0x02, 0x02, 0x84, 0x6A, 0x96, 0x00};
static ip4_addr_t s_ipaddr;
static ip4_addr_t s_netmask;
static ip4_addr_t s_gateway;

static dhcp_entry_t s_dhcp_entries[3];
static dhcp_config_t s_dhcp_config;

// Declarações prévias para fluxo de flush de rede e encerramento limpo de conexões HTTP
static void service_traffic(void);
static void send_raw_data(struct tcp_pcb *tpcb, const char *content_type, const void *data, int content_len);
static void send_html_page(struct tcp_pcb *tpcb, const char *page);

/**
 * @brief Inicializa o endereço MAC de hardware do adaptador USB Ethernet.
 * 
 * Extrai o identificador único gravado a laser no chip Flash NOR do RP2040
 * (pico_get_unique_board_id). O bit de administração local é setado (0x02)
 * e o último byte é derivado do ID de hardware.
 */
void rede_init_mac(void) {
    pico_unique_board_id_t u_id;
    pico_get_unique_board_id(&u_id);
    tud_network_mac_address[0] = 0x02; // Bit 1 = 1 (administrado localmente), Bit 0 = 0 (unicast)
    tud_network_mac_address[1] = 0x02;
    tud_network_mac_address[2] = 0x84;
    tud_network_mac_address[3] = u_id.id[5];
    tud_network_mac_address[4] = u_id.id[6];
    tud_network_mac_address[5] = u_id.id[7];
}

/**
 * @brief Executa a sequência de reinicialização segura do dispositivo.
 * 
 * Passos executados:
 *   1. Drena a pilha USB e TCP/IP por 1200ms para enviar a resposta HTTP pendente ao host.
 *   2. Desconecta o barramento USB (tud_disconnect) por 1200ms para forçar o Windows a
 *      destruir a sessão NDIS e liberar a interface de rede em cache.
 *   3. Dispara o Watchdog de hardware e, subsidiariamente, aciona SYSRESETREQ no Cortex-M0+.
 */
void rede_reboot_system(void) {
    // 1. Drena tarefas TinyUSB e lwIP por 1200ms para entregar a resposta HTTP completa ao host e fechar o socket TCP (FIN/ACK)
    absolute_time_t flush_until = make_timeout_time_ms(1200);
    while (!time_reached(flush_until)) {
        tud_task();
        service_traffic();
        sleep_ms(2);
    }

    // 2. Desconecta fisicamente o barramento USB desativando o pull-up D+
    // Eletricamente, para o host USB (Windows/macOS/Linux), equivale a retirar o cabo da porta
    tud_disconnect();

    // 3. Mantem a desconexao eletrica por 1200ms para o Windows destruir totalmente a sessao NDIS
    sleep_ms(1200);

    // 4. Inicia o gerador de pulsos do Watchdog (12 MHz padrão RP2040)
    watchdog_start_tick(12);

    // 5. Aciona o reinício imediato por Watchdog por hardware (delay 0 ms)
    watchdog_reboot(0, 0, 0);

    // 6. Fallback garantido: ARM Cortex-M0+ Hardware System Reset (SYSRESETREQ)
    *((io_rw_32 *)(PPB_BASE + M0PLUS_AIRCR_OFFSET)) = 0x05FA0000 | M0PLUS_AIRCR_SYSRESETREQ_BITS;

    while (true) {
        tight_loop_contents();
    }
}

/**
 * @brief Callback de saída de pacotes Ethernet da pilha lwIP para o driver USB TinyUSB.
 * 
 * @param netif Ponteiro para a interface de rede lwIP.
 * @param p Buffer pbuf contendo o pacote Ethernet a ser transmitido.
 * @return err_t ERR_OK em sucesso, ou código de erro lwIP.
 */
static err_t linkoutput_fn(struct netif *netif, struct pbuf *p) {
    (void) netif;
    absolute_time_t timeout = make_timeout_time_ms(2000);
    while (!time_reached(timeout)) {
        if (!tud_ready()) return ERR_USE;
        if (tud_network_can_xmit(p->tot_len)) {
            tud_network_xmit(p, 0);
            return ERR_OK;
        }
        tud_task();
        watchdog_update();
    }
    return ERR_WOULDBLOCK;
}

/**
 * @brief Callback de saída IPv4 que resolve ARP e encaminha o pacote.
 * 
 * @param netif Interface de rede lwIP.
 * @param p Buffer pbuf do pacote IP.
 * @param addr Endereço IPv4 de destino.
 * @return err_t Resultado da operação de envio via etharp_output.
 */
static err_t ip4_output_fn(struct netif *netif, struct pbuf *p, const ip4_addr_t *addr) {
    return etharp_output(netif, p, addr);
}

/**
 * @brief Callback de configuração da interface de rede virtual lwIP.
 * 
 * @param netif Ponteiro para a estrutura netif a ser inicializada.
 * @return err_t ERR_OK em sucesso.
 */
static err_t netif_init_cb(struct netif *netif) {
    netif->mtu = CFG_TUD_NET_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->state = NULL;
    netif->name[0] = 'E';
    netif->name[1] = 'X';
    netif->linkoutput = linkoutput_fn;
    netif->output = ip4_output_fn;
    return ERR_OK;
}

/**
 * @brief Processador de consultas do servidor DNS captive portal.
 * 
 * @param name Nome de domínio consultado (ex: "pico.artnet", "setup.local").
 * @param addr Endereço IP retornado para redirecionamento ao nó.
 * @return true se a consulta foi reconhecida e respondida; false caso contrário.
 */
bool dns_query_proc(const char *name, ip4_addr_t *addr) {
    if (0 == strcmp(name, "pico.artnet") || 0 == strcmp(name, "artnet.local") ||
        0 == strcmp(name, "setup.local")  || 0 == strcmp(name, "config.local") ||
        0 == strcmp(name, "tiny.usb")) {
        *addr = s_ipaddr;
        return true;
    }
    return false;
}

/**
 * @brief Callback chamado pelo TinyUSB quando um pacote Ethernet chega pelo barramento USB.
 * 
 * Aloca um pbuf na pilha lwIP e enfileira na fila circular de recepção para
 * processamento assíncrono seguro sem bloquear as interrupções de hardware USB.
 * 
 * @param src Ponteiro para os dados Ethernet recebidos.
 * @param size Tamanho do pacote em bytes.
 * @return true para confirmar recepção e renovar o buffer USB; false se a fila estiver cheia.
 */
bool tud_network_recv_cb(const uint8_t *src, uint16_t size) {
    if (recv_queue_is_full()) {
        return false; // Fila cheia, TinyUSB descartará e renovará
    }
    if (size) {
        struct pbuf *p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);
        if (p) {
            pbuf_take(p, src, size);
            s_recv_queue[s_recv_head] = p;
            s_recv_head = (s_recv_head + 1) % RECV_QUEUE_SIZE;
        }
    }
    tud_network_recv_renew();
    return true;
}

/**
 * @brief Callback chamado pelo TinyUSB para preencher o buffer de transmissão USB.
 * 
 * @param dst Buffer de destino USB.
 * @param ref Ponteiro para o pbuf da mensagem Ethernet a ser transmitida.
 * @param arg Argumento reservado.
 * @return uint16_t Quantidade de bytes efetivamente copiados para o buffer USB.
 */
uint16_t tud_network_xmit_cb(uint8_t *dst, void *ref, uint16_t arg) {
    (void) arg;
    struct pbuf *p = (struct pbuf *) ref;
    return pbuf_copy_partial(p, dst, p->tot_len, 0);
}

/**
 * @brief Callback de reinicialização da interface de rede TinyUSB.
 * 
 * Esvazia a fila circular de pbufs pendentes e desaloca a memória na lwIP.
 */
void tud_network_init_cb(void) {
    while (!recv_queue_is_empty()) {
        struct pbuf *p = s_recv_queue[s_recv_tail];
        s_recv_tail = (s_recv_tail + 1) % RECV_QUEUE_SIZE;
        if (p) {
            pbuf_free(p);
        }
    }
}

// Normaliza pacotes Art-Net UDP (porta 6454) enviados para qualquer endereço de broadcast
// redirecionando para s_ipaddr local para que o lwIP os entregue confiavelmente ao socket
/**
 * @brief Normaliza pacotes Art-Net UDP (porta 6454) enviados para endereços de broadcast.
 * 
 * Softwares de iluminação frequentemente transmitem para o broadcast da sub-rede
 * ou broadcast limitado (255.255.255.255). Esta rotina ajusta o destino IPv4
 * para o IP local do nó no cabeçalho do pacote para assegurar entrega imediata ao socket UDP.
 * 
 * @param p Buffer pbuf com o frame Ethernet recebido.
 */
static void patch_artnet_broadcast(struct pbuf *p) {
    if (!p || p->tot_len < 14 + 20 + 8) return;
    uint8_t *data = (uint8_t *)p->payload;
    if (data[12] == 0x08 && data[13] == 0x00) { // IPv4
        uint8_t ihl = (data[14] & 0x0F) * 4;
        if (ihl >= 20 && p->tot_len >= (uint16_t)(14 + ihl + 8)) {
            uint8_t proto = data[23];
            if (proto == 17) { // UDP
                uint16_t dport = (data[14 + ihl + 2] << 8) | data[14 + ihl + 3];
                if (dport == 6454) { // Art-Net
                    const uint8_t *ip_b = (const uint8_t *)&s_ipaddr;
                    data[30] = ip_b[0];
                    data[31] = ip_b[1];
                    data[32] = ip_b[2];
                    data[33] = ip_b[3];

                    data[24] = 0;
                    data[25] = 0;
                    uint32_t sum = 0;
                    for (int i = 0; i < ihl; i += 2) {
                        sum += ((uint16_t)data[14 + i] << 8) | data[14 + i + 1];
                    }
                    while (sum >> 16) {
                        sum = (sum & 0xFFFF) + (sum >> 16);
                    }
                    uint16_t cksum = (uint16_t)~sum;
                    data[24] = (cksum >> 8) & 0xFF;
                    data[25] = cksum & 0xFF;

                    data[14 + ihl + 6] = 0x00;
                    data[14 + ihl + 7] = 0x00;
                }
            }
        }
    }
}

/**
 * @brief Drena e processa pacotes pendentes na fila circular de recepção de rede.
 * 
 * Retira os pbufs da fila `s_recv_queue`, aplica a correção de broadcast Art-Net
 * e entrega à pilha lwIP chamando `s_netif_data.input()`.
 */
static void service_traffic(void) {
    while (!recv_queue_is_empty()) {
        struct pbuf *p = s_recv_queue[s_recv_tail];
        s_recv_tail = (s_recv_tail + 1) % RECV_QUEUE_SIZE;
        patch_artnet_broadcast(p);
        ethernet_input(p, &s_netif_data);
        watchdog_update();
    }
    sys_check_timeouts();
}

static bool s_usb_net_initialized = false;
/**
 * @brief Inicializa os parâmetros de rede IP, servidor DHCP e servidor DNS.
 * 
 * Configura:
 *   - Modo Setup Inicial: IP estático 10.10.0.1, faixa DHCP 10.10.0.2 a 10.10.0.3, DNS ativo.
 *   - Modo Operacional: IP estático 2.0.N.1 (onde N é o node_id), faixa DHCP 2.0.N.2 a 2.0.N.3.
 */
static void init_usb_ethernet(void) {
    if (s_usb_net_initialized) return;
    s_usb_net_initialized = true;

    uint8_t ip_b0, ip_b1, ip_b2, ip_b3;
    if (!g_config.configured) {
        // Modo de Configuração Inicial / Assistente
        ip_b0 = 10;
        ip_b1 = 10;
        ip_b2 = 0;
        ip_b3 = 1;
        snprintf(s_ip_str, sizeof(s_ip_str), "10.10.0.1");
        printf("[REDE] MODO SETUP ATIVO: IP %s (Aguardando configuracao de identidade)\n", s_ip_str);
    } else {
        // Modo Operacional Ativo (Nó 1 a 16)
        uint8_t n = g_config.node_id;
        if (n < 1 || n > 16) n = 1;
        ip_b0 = 2;
        ip_b1 = 0;
        ip_b2 = n;
        ip_b3 = 1;
        snprintf(s_ip_str, sizeof(s_ip_str), "2.0.%u.1", n);
        printf("[REDE] MODO OPERACIONAL ATIVO: No %02u | IP %s | Universos %u a %u\n",
               n, s_ip_str, g_config.start_universe, g_config.start_universe + 7);
    }

    IP4_ADDR(&s_ipaddr,  ip_b0, ip_b1, ip_b2, ip_b3);
    IP4_ADDR(&s_netmask, 255,   255,   255,   0);
    IP4_ADDR(&s_gateway, 0,     0,     0,     0);

    memset(s_dhcp_entries, 0, sizeof(s_dhcp_entries));
    IP4_ADDR(&s_dhcp_entries[0].addr, ip_b0, ip_b1, ip_b2, 2);
    s_dhcp_entries[0].lease = 24 * 60 * 60;
    IP4_ADDR(&s_dhcp_entries[1].addr, ip_b0, ip_b1, ip_b2, 3);
    s_dhcp_entries[1].lease = 24 * 60 * 60;
    IP4_ADDR(&s_dhcp_entries[2].addr, ip_b0, ip_b1, ip_b2, 4);
    s_dhcp_entries[2].lease = 24 * 60 * 60;

    IP4_ADDR(&s_dhcp_config.router, 0, 0, 0, 0);
    s_dhcp_config.port = 67;
    IP4_ADDR(&s_dhcp_config.dns, ip_b0, ip_b1, ip_b2, ip_b3);
    s_dhcp_config.domain = "usb";
    s_dhcp_config.num_entry = 3;
    s_dhcp_config.entries = s_dhcp_entries;

    struct netif *netif = &s_netif_data;
    netif->hwaddr_len = sizeof(tud_network_mac_address);
    memcpy(netif->hwaddr, tud_network_mac_address, sizeof(tud_network_mac_address));
    netif->hwaddr[5] ^= 0x01; // Inverte o último bit para MAC virtual

    netif_add(netif, &s_ipaddr, &s_netmask, &s_gateway, NULL, netif_init_cb, ip_input);
    netif_set_default(netif);
    netif_set_up(netif);

    // Servidor DHCP atribui o IP .2 ao PC conectado
    dhserv_init(&s_dhcp_config);
    dnserv_init(IP_ADDR_ANY, 53, dns_query_proc);

    s_connected = true;

    artnet_start_listener();
    http_server_start();
}

// ============================================================================
// PÁGINAS HTML PROFISSIONAIS MULTI-IDIOMA (PT / EN / ES / ZERO EMOJIS)
// ============================================================================

// 1. Página de Configuração SPA em 3 Passos (Modo Setup e Menu /config)
#include "html_setup_spa.h"

// 2. Dashboard Principal ao Vivo (Rota padrão '/' quando configurado)
#include "html_dashboard.h"

/**
 * @brief Renderiza e envia a página HTML de confirmação de salvamento e contagem regressiva de reinício.
 * 
 * @param tpcb Ponteiro para o PCB TCP da conexão ativa.
 * @param node_id ID configurado do nó para cálculo do IP de destino na dashboard.
 */
static void send_save_page(struct tcp_pcb *tpcb, uint8_t node_id) {
    char new_ip[32];
    snprintf(new_ip, sizeof(new_ip), "2.0.%u.1", node_id);
    uint16_t u_cnt = (g_config.def_universos >= 1 && g_config.def_universos <= 8) ? g_config.def_universos : 8;
    uint16_t start_u = (node_id - 1) * u_cnt;
    uint16_t end_u = start_u + (u_cnt > 0 ? u_cnt - 1 : 0);

    const web_reboot_strings_t *w = lang_get_web(g_config.language);

    char save_msg_buf[256];
    snprintf(save_msg_buf, sizeof(save_msg_buf), w->save_msg, node_id, start_u, end_u);

    static char s_save_page_buf[2560];
    snprintf(s_save_page_buf, sizeof(s_save_page_buf),
        "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        "<meta http-equiv='refresh' content='16;url=http://%s/'>"
        "<title>Pico ArtNet</title>"
        "<style>"
        ":root { --bg:#eef2f6; --card-bg:#ffffff; --card-border:#e2e8f0; --text:#0f172a; --text-sub:#64748b; }"
        "[data-theme='dark'] { --bg:#090d16; --card-bg:#111827; --card-border:#1f2937; --text:#f8fafc; --text-sub:#94a3b8; }"
        "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; margin:0; padding:30px 14px; background:var(--bg); color:var(--text); text-align:center; }"
        ".card { max-width:440px; margin:40px auto; background:var(--card-bg); border:1px solid var(--card-border); border-radius:16px; box-shadow:0 10px 25px rgba(0,0,0,0.06); padding:32px 24px; }"
        "h2 { color:var(--text); margin:16px 0 8px; font-size:22px; }"
        "p { color:var(--text-sub); font-size:14px; margin:6px 0; line-height:1.5; }"
        ".ip-badge { display:inline-block; background:rgba(2,132,199,0.12); border:1px solid rgba(2,132,199,0.3); color:#0284c7; font-weight:700; font-size:16px; font-family:monospace; padding:8px 16px; border-radius:8px; margin:16px 0; }"
        ".progress-bg { background:var(--card-border); border-radius:999px; height:8px; overflow:hidden; margin:22px 0 10px; }"
        ".progress-fill { background:#10b981; height:100%%; width:0%%; animation:fill 10s linear forwards; }"
        "@keyframes fill { 0%% { width:0%%; } 100%% { width:100%%; } }"
        ".btn-link { display:inline-block; margin-top:16px; padding:10px 20px; background:#0284c7; color:#fff; border-radius:8px; text-decoration:none; font-size:14px; font-weight:600; }"
        ".btn-link:hover { background:#0369a1; }"
        "</style>"
        "<script>"
        "function applyAutoTheme(){var isDark=window.matchMedia&&window.matchMedia('(prefers-color-scheme: dark)').matches;document.documentElement.setAttribute('data-theme',isDark?'dark':'light');}applyAutoTheme();"
        "var targetUrl='http://%s/';var s=10;var redirected=false;"
        "function doRedirect(){if(redirected)return;redirected=true;window.location.replace(targetUrl);}"
        "function checkAndRedirect(){var p=document.getElementById('cnt-wrap');if(p)p.innerHTML='<span style=\"color:#0284c7;font-weight:600;\">Aguardando rede USB... Redirecionando...</span>';"
        "fetch(targetUrl,{mode:'no-cors',cache:'no-store'}).then(function(){doRedirect();}).catch(function(){setTimeout(checkAndRedirect,1000);});}"
        "var timer=setInterval(function(){s--;var el=document.getElementById('cnt');if(el)el.innerText=s;if(s<=0){clearInterval(timer);checkAndRedirect();}},1000);"
        "</script>"
        "</head><body>"
        "<div class='card'>"
        "<svg width='48' height='48' viewBox='0 0 24 24' fill='none' stroke='#10b981' stroke-width='2.5' stroke-linecap='round' stroke-linejoin='round'><path d='M22 11.08V12a10 10 0 1 1-5.93-9.14'></path><polyline points='22 4 12 14.01 9 11.01'></polyline></svg>"
        "<h2>%s</h2>"
        "<p>%s</p>"
        "<div class='ip-badge'>http://%s/</div>"
        "<p>%s</p>"
        "<div class='progress-bg'><div class='progress-fill'></div></div>"
        "<p id='cnt-wrap' style='font-size:13px; color:var(--text-sub);'>%s</p>"
        "<a href='http://%s/' class='btn-link'>%s</a>"
        "</div></body></html>",
        new_ip, new_ip, w->save_title, save_msg_buf, new_ip, w->reboot_wait, w->redirecting, new_ip, w->btn_manual);

    send_html_page(tpcb, s_save_page_buf);
}

/**
 * @brief Renderiza e envia a página HTML informativa de restauração dos padrões de fábrica.
 * 
 * @param tpcb Ponteiro para o PCB TCP da conexão ativa.
 */
static void send_factory_reset_page(struct tcp_pcb *tpcb) {
    static char s_reset_page_buf[2048];
    snprintf(s_reset_page_buf, sizeof(s_reset_page_buf),
        "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        "<meta http-equiv='refresh' content='12;url=http://10.10.0.1/'>"
        "<title>Pico ArtNet</title>"
        "<style>"
        ":root { --bg:#eef2f6; --card-bg:#ffffff; --card-border:#e2e8f0; --text:#0f172a; --text-sub:#64748b; }"
        "[data-theme='dark'] { --bg:#090d16; --card-bg:#111827; --card-border:#1f2937; --text:#f8fafc; --text-sub:#94a3b8; }"
        "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; margin:0; padding:30px 14px; background:var(--bg); color:var(--text); text-align:center; }"
        ".card { max-width:440px; margin:40px auto; background:var(--card-bg); border:1px solid var(--card-border); border-radius:16px; box-shadow:0 10px 25px rgba(0,0,0,0.06); padding:32px 24px; }"
        "h2 { color:var(--text); margin:16px 0 8px; font-size:22px; }"
        "p { color:var(--text-sub); font-size:14px; margin:6px 0; line-height:1.5; }"
        ".ip-badge { display:inline-block; background:rgba(2,132,199,0.12); border:1px solid rgba(2,132,199,0.3); color:#0284c7; font-weight:700; font-size:16px; font-family:monospace; padding:8px 16px; border-radius:8px; margin:16px 0; }"
        ".progress-bg { background:var(--card-border); border-radius:999px; height:8px; overflow:hidden; margin:22px 0 10px; }"
        ".progress-fill { background:#0284c7; height:100%%; width:0%%; animation:fill 7s linear forwards; }"
        "@keyframes fill { 0%% { width:0%%; } 100%% { width:100%%; } }"
        ".spinner { width:44px; height:44px; border:4px solid var(--card-border); border-top-color:#0284c7; border-radius:50%%; animation:spin 1s linear infinite; margin:0 auto 16px; }"
        "@keyframes spin { 100%% { transform:rotate(360deg); } }"
        ".btn-link { display:inline-block; margin-top:16px; padding:10px 20px; background:#0284c7; color:#fff; border-radius:8px; text-decoration:none; font-size:14px; font-weight:600; }"
        ".btn-link:hover { background:#0369a1; }"
        "</style>"
        "<script>"
        "function applyAutoTheme(){var isDark=window.matchMedia&&window.matchMedia('(prefers-color-scheme: dark)').matches;document.documentElement.setAttribute('data-theme',isDark?'dark':'light');}applyAutoTheme();"
        "var targetUrl='http://10.10.0.1/';var redirected=false;var readyCalled=false;"
        "function doRedirect(){if(redirected)return;redirected=true;window.location.replace(targetUrl);}"
        "function onNodeReady(){if(readyCalled)return;readyCalled=true;var h=document.getElementById('r-title');if(h)h.innerText='Salvo!';var s=document.getElementById('r-sub');if(s)s.innerText='Salvo! Acessando página do dispositivo...';var bar=document.getElementById('r-bar');if(bar){bar.style.animation='none';bar.style.width='100%%';bar.style.background='#10b981';}var ic=document.getElementById('r-icon');if(ic){ic.innerHTML=\"<svg width='48' height='48' viewBox='0 0 24 24' fill='none' stroke='#10b981' stroke-width='2.5' stroke-linecap='round' stroke-linejoin='round'><path d='M22 11.08V12a10 10 0 1 1-5.93-9.14'></path><polyline points='22 4 12 14.01 9 11.01'></polyline></svg>\";}setTimeout(doRedirect,800);}"
        "var probeTries=0;"
        "function pingNode(){if(readyCalled)return;if(probeTries>=3)return;probeTries++;fetch(targetUrl,{mode:'no-cors',cache:'no-store'}).then(function(){onNodeReady();}).catch(function(){if(!readyCalled&&probeTries<3)setTimeout(pingNode,1200);});}"
        "setTimeout(pingNode,2500);"
        "setTimeout(function(){onNodeReady();},7000);"
        "</script>"
        "</head><body>"
        "<div class='card'>"
        "<div id='r-icon'><div class='spinner'></div></div>"
        "<h2 id='r-title'>Salvando e reiniciando...</h2>"
        "<p id='r-sub'>Aguardando o dispositivo inicializar em 10.10.0.1...</p>"
        "<div class='ip-badge'>http://10.10.0.1/</div>"
        "<div class='progress-bg'><div id='r-bar' class='progress-fill'></div></div>"
        "<a href='http://10.10.0.1/' class='btn-link'>Acessar Diretamente</a>"
        "</div></body></html>"
    );

    send_html_page(tpcb, s_reset_page_buf);
}

static const char HTML_REBOOT_PAGE[] =
"<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
"<meta http-equiv='refresh' content='16;url=/'>"
"<title>Pico ArtNet</title>"
"<style>"
":root { --bg:#eef2f6; --card-bg:#ffffff; --card-border:#e2e8f0; --text:#0f172a; --text-sub:#64748b; }"
"[data-theme='dark'] { --bg:#090d16; --card-bg:#111827; --card-border:#1f2937; --text:#f8fafc; --text-sub:#94a3b8; }"
"body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; text-align:center; padding:50px 14px; background:var(--bg); color:var(--text); transition:background-color 0.25s; }"
".card { background:var(--card-bg); padding:32px 24px; border-radius:16px; border:1px solid var(--card-border); max-width:400px; margin:auto; box-shadow:0 10px 25px rgba(0,0,0,0.06); }"
".progress-bg { background:var(--card-border); border-radius:999px; height:8px; overflow:hidden; margin:22px 0 10px; }"
".progress-fill { background:#0284c7; height:100%; width:0%; animation:fill 10s linear forwards; }"
"@keyframes fill { 0% { width:0%; } 100% { width:100%; } }"
".btn-link { display:inline-block; margin-top:16px; padding:10px 20px; background:#0284c7; color:#fff; border-radius:8px; text-decoration:none; font-size:14px; font-weight:600; }"
".btn-link:hover { background:#0369a1; }"
"</style>"
"<script>"
"function applyAutoTheme(){var isDark=window.matchMedia&&window.matchMedia('(prefers-color-scheme: dark)').matches;document.documentElement.setAttribute('data-theme',isDark?'dark':'light');}applyAutoTheme();"
"var targetUrl='/';var s=10;var redirected=false;"
"function doRedirect(){if(redirected)return;redirected=true;window.location.replace(targetUrl);}"
"function checkAndRedirect(){var p=document.getElementById('cnt-wrap');if(p)p.innerHTML='<span style=\"color:#0284c7;font-weight:600;\">Aguardando rede USB... Redirecionando...</span>';"
"fetch(targetUrl,{mode:'no-cors',cache:'no-store'}).then(function(){doRedirect();}).catch(function(){setTimeout(checkAndRedirect,1000);});}"
"var timer=setInterval(function(){s--;var el=document.getElementById('cnt');if(el)el.innerText=s;if(s<=0){clearInterval(timer);checkAndRedirect();}},1000);"
"</script>"
"</head><body><div class='card'>"
"<h3>Reiniciando...</h3>"
"<p style='color:var(--text-sub);'>O microcontrolador Pico ArtNet está reiniciando.</p>"
"<div class='progress-bg'><div class='progress-fill'></div></div>"
"<p id='cnt-wrap' style='color:var(--text-sub); font-size:13px;'>Redirecionando em <span id='cnt' style='font-weight:700; color:#0284c7;'>10</span> segundos...</p>"
"<a href='/' class='btn-link'>Acessar Diretamente</a>"
"</div></body></html>";

/**
 * @brief Decodifica uma string codificada em percent-encoding (URL Encoding).
 * 
 * Converte caracteres como %20 em espaço, %2B em '+', etc.
 * 
 * @param dst Buffer de destino da string decodificada.
 * @param src String de origem codificada.
 * @param max_len Tamanho máximo do buffer de destino.
 */
static void url_decode(char *dst, const char *src, size_t max_len) {
    size_t out_len = 0;
    while (*src && out_len < max_len - 1) {
        if (*src == '+') {
            dst[out_len++] = ' ';
            src++;
        } else if (*src == '%' && src[1] && src[2]) {
            char hex[3] = {src[1], src[2], 0};
            dst[out_len++] = (char)strtol(hex, NULL, 16);
            src += 3;
        } else {
            dst[out_len++] = *src++;
        }
    }
    dst[out_len] = '\0';
}

/**
 * @brief Extrai o valor de um parâmetro de formulário HTTP GET ou POST URL-encoded.
 * 
 * @param req Ponteiro para a requisição HTTP completa.
 * @param param Nome do parâmetro a localizar (ex: "nome=", "universos=").
 * @param dst Buffer de destino para o valor extraído e decodificado.
 * @param max_len Capacidade máxima do buffer de destino.
 */
static void extract_param(const char *req, const char *param, char *dst, size_t max_len) {
    const char *p = strstr(req, param);
    if (!p) {
        dst[0] = '\0';
        return;
    }
    p += strlen(param);
    char raw[64];
    size_t i = 0;
    while (*p && *p != '&' && *p != ' ' && *p != '\r' && *p != '\n' && i < sizeof(raw) - 1) {
        raw[i++] = *p++;
    }
    raw[i] = '\0';
    url_decode(dst, raw, max_len);
}

// ============================================================================
// RECEPTOR UDP ART-NET (PORTA 6454)
// ============================================================================
/**
 * @brief Callback de recepção de pacotes Art-Net UDP na porta 6454.
 * 
 * Realiza a análise estruturada dos pacotes de acordo com o protocolo Art-Net v4:
 *   - Valida cabeçalho "Art-Net\0" (8 bytes).
 *   - OpPoll (0x2000): Responde com OpPollReply contendo o nome do nó, portas físicas,
 *     universos configurados, IP, MAC e telemetria de funcionamento.
 *   - OpDmx (0x5000): Valida o universo de destino. Se pertencer à faixa deste nó,
 *     atualiza os contadores de telemetria (PPS/FPS), extrai a sequência e entrega
 *     os dados DMX ao subsistema de transmissão (`dmx_set_universe_data`).
 *   - OpSync (0x5200): Dispara o commit simultâneo de todos os universos retidos (`dmx_commit_artsync`).
 * 
 * @param arg Argumento de usuário.
 * @param upcb PCB UDP que recebeu o datagrama.
 * @param p Buffer pbuf contendo o datagrama UDP.
 * @param addr Endereço IP do transmissor do pacote.
 * @param port Porta UDP de origem do transmissor.
 */
static void artnet_udp_recv_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    (void) arg;
    if (p == NULL) return;

    // Se o nó ainda estiver no modo de configuração inicial (10.10.0.1), Art-Net fica dormente
    if (!g_config.configured) {
        pbuf_free(p);
        return;
    }

    if (p->len >= 10) {
        uint8_t *payload = (uint8_t *)p->payload;
        if (memcmp(payload, "Art-Net\0", 8) == 0) {
            uint16_t opcode = payload[8] | (payload[9] << 8);
            uint32_t now = to_ms_since_boot(get_absolute_time());

            // Timeout automatico para ArtSync: se passar mais de 4s sem OpSync, volta ao modo imediato
            if (g_artsync_active && (now - s_last_artsync_time > 4000)) {
                g_artsync_active = false;
            }

            // 1. ArtDmx (0x5000): Atualizacao de canais DMX
            if (opcode == 0x5000 && p->len >= 18) {
                // Se o modo de teste manual de faders (/tester) estiver ativo,
                // suspende o processamento de pacotes Art-Net para priorizar 100% os faders
                if (g_manual_test_active) {
                    pbuf_free(p);
                    return;
                }
                uint16_t universe = payload[14] | (payload[15] << 8);
                uint16_t start_u = g_config.start_universe;

                // Processa exclusivamente o bloco de 8 universos mapeado para este nó!
                // Ex: Nó 1 (start_universe = 0) processa 0..7
                //     Nó 2 (start_universe = 8) processa 8..15
                // Pacotes de outros universos são descartados instantaneamente sem carga de CPU
                uint16_t num_u = (g_config.def_universos >= 1 && g_config.def_universos <= 8) ? g_config.def_universos : 8;
                if (universe < start_u || universe >= start_u + num_u) {
                    pbuf_free(p);
                    return;
                }

                uint16_t u_idx = universe - start_u; // Índice físico local (0 a 7)
                uint8_t seq = payload[12]; // Sequence (1..255; 0 desativa verificacao)
                if (seq != 0) {
                    if (s_has_sequence[u_idx]) {
                        uint8_t expected = s_last_sequence[u_idx] + 1;
                        if (expected == 0) expected = 1;
                        if (seq != expected) {
                            uint8_t diff = (seq > s_last_sequence[u_idx]) ?
                                           (seq - s_last_sequence[u_idx] - 1) :
                                           (255 - s_last_sequence[u_idx] + seq - 1);
                            if (diff > 0 && diff < 50) {
                                g_artnet_lost_packets += diff;
                            }
                        }
                    }
                    s_last_sequence[u_idx] = seq;
                    s_has_sequence[u_idx] = true;
                }

                uint16_t length = (payload[16] << 8) | payload[17];
                uint8_t *dmx_data = &payload[18];
                dmx_set_universe_data(u_idx, dmx_data, length, g_artsync_active);

                // Telemetria
                g_artnet_packets_total++;
                g_artnet_packets_sec++;
                g_artnet_active_universes_mask |= (1 << (u_idx & 7));
                g_artnet_last_universe = universe;
                g_artnet_active = true;
                g_artnet_last_packet_time = now;
                if (length >= 1) g_artnet_ch1 = dmx_data[0];
                if (length >= 2) g_artnet_ch2 = dmx_data[1];
                if (length >= 3) g_artnet_ch3 = dmx_data[2];
            }
            // 2. ArtSync (0x5200): Sincronizacao simultanea de todos os universos
            else if (opcode == 0x5200) {
                g_artsync_active = true;
                s_last_artsync_time = now;
                dmx_commit_artsync();
            }
            // 3. ArtPoll (0x2000): Descoberta automatica por mesas de luz e softwares (Art-Net 4 Dual PollReply)
            else if (opcode == 0x2000) {
                int total_u = (g_config.def_universos >= 1 && g_config.def_universos <= 8) ? g_config.def_universos : 8;
                int max_b = (total_u > 4) ? 2 : 1;
                for (int b = 1; b <= max_b; b++) {
                    uint8_t reply[239];
                    memset(reply, 0, sizeof(reply));
                    memcpy(&reply[0], "Art-Net\0", 8);
                    reply[8] = 0x00; reply[9] = 0x21; // OpPollReply (0x2100)

                    const uint8_t *ip_b = (const uint8_t *)&s_ipaddr;
                    reply[10] = ip_b[0]; reply[11] = ip_b[1]; reply[12] = ip_b[2]; reply[13] = ip_b[3];
                    reply[14] = 0x36; reply[15] = 0x19; // Porta 6454 (0x1936 little endian)
                    reply[16] = 0x00; reply[17] = 0x02; // Versão de firmware 2.0
                    reply[18] = (g_config.start_universe >> 8) & 0x7F; // NetSwitch
                    reply[19] = (g_config.start_universe >> 4) & 0x0F; // SubSwitch
                    reply[20] = 0x00; reply[21] = 0xFF; // OEM code
                    reply[22] = 0x00;                   // UbeaVersion
                    reply[23] = 0xD0;                   // Status1
                    reply[24] = 0x00; reply[25] = 0x00; // ESTA code

                    // ShortName (máx 17 caracteres + null)
                    char short_name[18];
                    if (g_config.node_name[0] != '\0') {
                        snprintf(short_name, sizeof(short_name), "%s", g_config.node_name);
                    } else {
                        snprintf(short_name, sizeof(short_name), "Pico ArtNet No %u", g_config.node_id);
                    }
                    strncpy((char *)&reply[26], short_name, 17);

                    // LongName (máx 63 caracteres + null)
                    char long_name[64];
                    int port_start = (b == 1) ? 1 : 5;
                    int port_count = (b == 1) ? (total_u > 4 ? 4 : total_u) : (total_u - 4);
                    int port_end = port_start + port_count - 1;
                    snprintf(long_name, sizeof(long_name), "%s (P%d-%d Univ %u-%u)",
                             short_name, port_start, port_end,
                             g_config.start_universe + (port_start - 1),
                             g_config.start_universe + (port_end - 1));
                    strncpy((char *)&reply[44], long_name, 63);

                    // NodeReport
                    char report[64];
                    snprintf(report, sizeof(report), "#0001 [%04lu] OK - No %02u P%d",
                             g_artnet_fps, g_config.node_id, b);
                    strncpy((char *)&reply[108], report, 63);

                    reply[172] = 0x00;
                    reply[173] = (uint8_t)port_count; // NumPorts
                    int u_base = (b - 1) * 4;
                    for (int u = 0; u < port_count; u++) {
                        reply[174 + u] = 0x80; // PortTypes: DMX512 Output
                        reply[178 + u] = 0x00; // GoodInput
                        reply[182 + u] = 0x80; // GoodOutput: Data transmitting
                        reply[186 + u] = (g_config.start_universe + u_base + u) & 0x0F; // SwIn
                        reply[190 + u] = (g_config.start_universe + u_base + u) & 0x0F; // SwOut
                    }
                    memcpy(&reply[201], tud_network_mac_address, 6); // MAC Address

                    // Art-Net 4 Bind fields
                    reply[207] = ip_b[0]; reply[208] = ip_b[1]; reply[209] = ip_b[2]; reply[210] = ip_b[3]; // BindIp
                    reply[211] = (uint8_t)b; // BindIndex (1 ou 2)
                    reply[212] = 0x08; // Status2: 15-bit port-address support

                    struct pbuf *rp = pbuf_alloc(PBUF_TRANSPORT, sizeof(reply), PBUF_RAM);
                    if (rp) {
                        memcpy(rp->payload, reply, sizeof(reply));
                        udp_sendto(upcb, rp, addr, port);
                        pbuf_free(rp);
                    }
                }
            }
        }
    }
    pbuf_free(p);
}

// ============================================================================
// SERVER-SENT EVENTS (SSE) - PUSH DE TELEMETRIA SEM SOBRECARGA DE POLLING
// ============================================================================
static void http_close_pcb(struct tcp_pcb *tpcb);
static struct tcp_pcb *s_sse_pcb = NULL;
static uint32_t s_last_sse_time = 0;
static uint8_t s_sse_req_u = 0;

static void http_sse_err_callback(void *arg, err_t err) {
    (void)arg; (void)err;
    s_sse_pcb = NULL;
}

static err_t http_sse_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    (void)arg; (void)err;
    if (!p) {
        if (s_sse_pcb == tpcb) s_sse_pcb = NULL;
        http_close_pcb(tpcb);
        return ERR_OK;
    }
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

static void http_sse_poll(void) {
    if (!s_sse_pcb) return;

    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - s_last_sse_time < 1000) return;
    s_last_sse_time = now;

    // Se o buffer de saída TCP estiver ocupado, descarta este ciclo sem bloquear
    // Garantia absoluta de que o tráfego Art-Net e DMX nunca sofram atraso
    if (tcp_sndbuf(s_sse_pcb) < 512) return;

    const uint8_t *active_buf = dmx_get_active_buffer((uint16_t)s_sse_req_u);
    uint8_t ch16[16] = {0};
    if (active_buf) {
        memcpy(ch16, &active_buf[1], 16);
    }

    static char s_sse_payload[512];
    int len = snprintf(s_sse_payload, sizeof(s_sse_payload),
        "data: {\"packets\":%lu,\"pps\":%lu,\"fps\":%lu,\"loss\":%lu,\"sync\":%s,"
        "\"univ\":%u,\"sel_u\":%d,\"max_u\":%d,\"start_u\":%u,\"loss_mode\":%u,\"dmx_rate\":%u,"
        "\"node_id\":%u,\"node_name\":\"%s\",\"ip\":\"%s\",\"lang\":%u,\"theme\":%u,\"active\":%s,"
        "\"ch\":[%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u]}\n\n",
        g_artnet_packets_total, g_artnet_pps, g_artnet_fps, g_artnet_lost_packets,
        g_artsync_active ? "true" : "false",
        g_artnet_last_universe, (int)s_sse_req_u, g_config.def_universos,
        g_config.start_universe, (unsigned)g_config.signal_loss_mode, (unsigned)g_config.dmx_rate,
        g_config.node_id, g_config.node_name, s_ip_str,
        (unsigned)g_config.language, (unsigned)g_config.theme,
        g_artnet_active ? "true" : "false",
        ch16[0], ch16[1], ch16[2], ch16[3],
        ch16[4], ch16[5], ch16[6], ch16[7],
        ch16[8], ch16[9], ch16[10], ch16[11],
        ch16[12], ch16[13], ch16[14], ch16[15]);

    err_t werr = tcp_write(s_sse_pcb, s_sse_payload, len, TCP_WRITE_FLAG_COPY);
    if (werr == ERR_OK) {
        tcp_output(s_sse_pcb);
    } else {
        http_close_pcb(s_sse_pcb);
        s_sse_pcb = NULL;
    }
}

// ============================================================================
// DMX STREAM BINÁRIO CONTÍNUO (PORTA 80) - STREAM PUSH DIRETO SEM REQUISIÇÕES REPETIDAS
// ============================================================================
static struct tcp_pcb *s_dmx_stream_pcb = NULL;
static uint32_t s_last_dmx_stream_time = 0;
static uint32_t s_dmx_stream_interval_ms = 500;
static uint8_t s_raw_dmx_stream_buf[16 + MAX_UNIVERSES * UNIVERSE_CHANNELS];

static void http_dmx_stream_err_callback(void *arg, err_t err) {
    (void)arg; (void)err;
    s_dmx_stream_pcb = NULL;
}

static err_t http_dmx_stream_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    (void)arg; (void)err;
    if (!p) {
        if (s_dmx_stream_pcb == tpcb) s_dmx_stream_pcb = NULL;
        http_close_pcb(tpcb);
        return ERR_OK;
    }
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

/**
 * @brief Rotina periódica de transmissão contínua do fluxo binário DMX (/stream).
 * 
 * Envia até 60 frames por segundo em formato binário bruto (512 bytes por universo)
 * para a interface web de monitoramento visual dos canais em tempo real.
 */
static void http_dmx_stream_poll(void) {
    if (!s_dmx_stream_pcb) return;
    if (s_dmx_stream_interval_ms == 0) return; // Pausado pelo usuário

    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - s_last_dmx_stream_time < s_dmx_stream_interval_ms) return;
    s_last_dmx_stream_time = now;

    // Prioridade máxima absoluta: se a fila TCP não tiver espaço para todo o frame,
    // descarta imediatamente sem bloquear a CPU nem as threads de Art-Net / DMX PIO
    if (tcp_sndbuf(s_dmx_stream_pcb) < sizeof(s_raw_dmx_stream_buf)) return;

    memcpy(&s_raw_dmx_stream_buf[0], (const void *)&g_artnet_packets_total, 4);
    memcpy(&s_raw_dmx_stream_buf[4], (const void *)&g_artnet_pps, 4);
    uint16_t fps16 = (uint16_t)g_artnet_fps;
    memcpy(&s_raw_dmx_stream_buf[8], &fps16, 2);
    s_raw_dmx_stream_buf[10] = (uint8_t)g_config.def_universos;
    s_raw_dmx_stream_buf[11] = g_config.language;
    s_raw_dmx_stream_buf[12] = 'D';
    s_raw_dmx_stream_buf[13] = 'M';
    s_raw_dmx_stream_buf[14] = 'X';
    s_raw_dmx_stream_buf[15] = '8';

    for (int u = 0; u < MAX_UNIVERSES; u++) {
        const uint8_t *abuf = dmx_get_active_buffer((uint16_t)u);
        if (abuf) {
            memcpy(&s_raw_dmx_stream_buf[16 + u * UNIVERSE_CHANNELS], &abuf[1], UNIVERSE_CHANNELS);
        } else {
            memset(&s_raw_dmx_stream_buf[16 + u * UNIVERSE_CHANNELS], 0, UNIVERSE_CHANNELS);
        }
    }

    err_t werr = tcp_write(s_dmx_stream_pcb, s_raw_dmx_stream_buf, sizeof(s_raw_dmx_stream_buf), TCP_WRITE_FLAG_COPY);
    if (werr == ERR_OK) {
        tcp_output(s_dmx_stream_pcb);
    } else {
        http_close_pcb(s_dmx_stream_pcb);
        s_dmx_stream_pcb = NULL;
    }
}

// ============================================================================
// SERVIDOR WEB HTTP (PORTA 80)
// ============================================================================
// ============================================================================
// SERVIDOR WEB HTTP (PORTA 80) COM SUPORTE MULTI-CONEXÃO E ZERO CORRUPÇÃO
// ============================================================================
#define HTTP_MAX_CONNS 16

typedef struct {
    struct tcp_pcb *pcb;
    const uint8_t *ptr;
    int remaining;
    uint8_t copy_flag;
    uint32_t birth_ms;
    bool in_use;
} http_conn_ctx_t;

static http_conn_ctx_t s_http_conns[HTTP_MAX_CONNS];

static err_t http_poll_close_callback(void *arg, struct tcp_pcb *tpcb) {
    (void)arg;
    if (tpcb) {
        if (tcp_close(tpcb) == ERR_OK) {
            return ERR_OK;
        }
    }
    return ERR_OK;
}

/**
 * @brief Encerra com segurança uma conexão TCP liberando os callbacks e buffers.
 * 
 * @param tpcb Ponteiro para o PCB TCP a ser encerrado.
 */
static void http_close_pcb(struct tcp_pcb *tpcb) {
    if (!tpcb) return;
    tcp_arg(tpcb, NULL);
    tcp_sent(tpcb, NULL);
    tcp_recv(tpcb, NULL);
    tcp_err(tpcb, NULL);
    tcp_poll(tpcb, NULL, 0);
    err_t err = tcp_close(tpcb);
    if (err != ERR_OK) {
        // Se tcp_close retornar ERR_MEM, não força tcp_abort (evita envio de RST/NS_ERROR_NET_RESET).
        // Agenda tentativa segura de fechamento no próximo ciclo do lwIP
        tcp_poll(tpcb, http_poll_close_callback, 1);
    }
}

static http_conn_ctx_t *http_alloc_conn(struct tcp_pcb *tpcb) {
    for (int i = 0; i < HTTP_MAX_CONNS; i++) {
        if (s_http_conns[i].in_use && s_http_conns[i].pcb == tpcb) {
            return &s_http_conns[i];
        }
    }
    for (int i = 0; i < HTTP_MAX_CONNS; i++) {
        if (!s_http_conns[i].in_use) {
            s_http_conns[i].in_use = true;
            s_http_conns[i].pcb = tpcb;
            s_http_conns[i].remaining = 0;
            s_http_conns[i].ptr = NULL;
            s_http_conns[i].birth_ms = to_ms_since_boot(get_absolute_time());
            return &s_http_conns[i];
        }
    }
    // Caso a tabela atinja o limite, recicla conexões antigas (> 20 segundos)
    uint32_t now = to_ms_since_boot(get_absolute_time());
    for (int i = 0; i < HTTP_MAX_CONNS; i++) {
        if (s_http_conns[i].in_use && (now - s_http_conns[i].birth_ms > 20000)) {
            if (s_http_conns[i].pcb) {
                http_close_pcb(s_http_conns[i].pcb);
            }
            s_http_conns[i].in_use = true;
            s_http_conns[i].pcb = tpcb;
            s_http_conns[i].remaining = 0;
            s_http_conns[i].ptr = NULL;
            s_http_conns[i].birth_ms = now;
            return &s_http_conns[i];
        }
    }
    return NULL;
}

static void http_free_conn(http_conn_ctx_t *ctx) {
    if (ctx) {
        ctx->in_use = false;
        ctx->pcb = NULL;
        ctx->remaining = 0;
        ctx->ptr = NULL;
        ctx->birth_ms = 0;
    }
}

static void http_err_callback(void *arg, err_t err) {
    (void) err;
    http_conn_ctx_t *ctx = (http_conn_ctx_t *)arg;
    if (ctx) {
        http_free_conn(ctx);
    }
}

/**
 * @brief Bombeia blocos parciais de dados HTTP para o socket TCP do cliente.
 * 
 * Implementa chunking não-bloqueante para envio de arquivos extensos (páginas HTML,
 * diagramas WebP) sem esgotar as janelas TCP do lwIP e sem travar a CPU.
 * 
 * @param ctx Contexto da conexão HTTP ativa.
 */
static void http_pump_conn(http_conn_ctx_t *ctx) {
    if (!ctx || !ctx->in_use || !ctx->pcb) return;
    struct tcp_pcb *tpcb = ctx->pcb;

    while (ctx->remaining > 0) {
        u16_t avail = tcp_sndbuf(tpcb);
        if (avail == 0) {
            break; // Buffer de saída cheio no momento, aguarda ACK do host via http_sent_callback
        }

        u16_t chunk = (ctx->remaining > 1460) ? 1460 : (u16_t)ctx->remaining;
        if (chunk > avail) chunk = avail;

        err_t werr = tcp_write(tpcb, ctx->ptr, chunk,
            ctx->copy_flag | (ctx->remaining > chunk ? TCP_WRITE_FLAG_MORE : 0));
        if (werr == ERR_OK) {
            ctx->ptr += chunk;
            ctx->remaining -= chunk;
        } else {
            break;
        }
    }

    tcp_output(tpcb);
    // Nota: O fechamento seguro ocorre exclusivamente no http_sent_callback quando todos os dados
    // transmitidos tiverem sido confirmados (ACK) pelo host (evitando NS_ERROR_NET_PARTIAL_TRANSFER).
}

static err_t http_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    (void) len;
    http_conn_ctx_t *ctx = (http_conn_ctx_t *)arg;
    if (ctx && ctx->in_use && ctx->pcb == tpcb) {
        // Atualiza timestamp de atividade para que downloads longos (ex: imagens) não sofram timeout
        ctx->birth_ms = to_ms_since_boot(get_absolute_time());

        if (ctx->remaining > 0) {
            http_pump_conn(ctx);
        } else if (tcp_sndbuf(tpcb) >= TCP_SND_BUF || tpcb->snd_queuelen == 0) {
            // Todos os dados foram transmitidos e confirmados pelo host!
            struct tcp_pcb *to_close = tpcb;
            http_free_conn(ctx);
            http_close_pcb(to_close);
        }
    }
    return ERR_OK;
}

// Resposta rápida para requisições de pacote único (< 1460 bytes)
// Aloca contexto e aguarda confirmação segura para fechamento sem RST
static void send_quick_response(struct tcp_pcb *tpcb, const char *content_type, const void *data, int data_len) {
    if (!tpcb) return;

    http_conn_ctx_t *ctx = http_alloc_conn(tpcb);
    if (!ctx) {
        http_close_pcb(tpcb);
        return;
    }

    tcp_arg(tpcb, ctx);
    tcp_err(tpcb, http_err_callback);
    tcp_sent(tpcb, http_sent_callback);

    char header[384];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Private-Network: true\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: *\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n", content_type, data_len);

    tcp_write(tpcb, header, hlen, TCP_WRITE_FLAG_COPY | (data_len > 0 ? TCP_WRITE_FLAG_MORE : 0));
    if (data && data_len > 0) {
        tcp_write(tpcb, data, data_len, TCP_WRITE_FLAG_COPY);
    }
    tcp_output(tpcb);

    ctx->ptr = NULL;
    ctx->remaining = 0;
    ctx->copy_flag = 0;
}

static void http_poll_conns(void) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    // Auto-release do modo de teste manual após 15 segundos sem atividade na página
    if (g_manual_test_active && (now - s_last_manual_test_time > 15000)) {
        g_manual_test_active = false;
    }

    for (int i = 0; i < HTTP_MAX_CONNS; i++) {
        if (s_http_conns[i].in_use && s_http_conns[i].pcb) {
            // Apenas descarta conexões que estejam travadas sem nenhuma atividade/ACK por mais de 20 segundos
            if (now - s_http_conns[i].birth_ms > 20000) {
                struct tcp_pcb *tpcb = s_http_conns[i].pcb;
                http_free_conn(&s_http_conns[i]);
                http_close_pcb(tpcb);
            }
        }
    }
}

// Envio assíncrono para payloads extensos (Flash ROM ou buffer grande)
static void send_raw_data(struct tcp_pcb *tpcb, const char *content_type, const void *data, int content_len) {
    if (!tpcb) return;

    if (content_len <= 1024) {
        send_quick_response(tpcb, content_type, data, content_len);
        return;
    }

    http_conn_ctx_t *ctx = http_alloc_conn(tpcb);
    if (!ctx) {
        http_close_pcb(tpcb);
        return;
    }

    tcp_arg(tpcb, ctx);
    tcp_err(tpcb, http_err_callback);
    tcp_sent(tpcb, http_sent_callback);

    char header[384];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Private-Network: true\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: *\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n", content_type, content_len);

    tcp_write(tpcb, header, hlen, TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE);

    uint8_t copy_flag = 0;
    uintptr_t addr = (uintptr_t)data;
    if (addr < 0x10000000 || addr >= 0x11000000) {
        copy_flag = TCP_WRITE_FLAG_COPY;
    }

    ctx->ptr = (const uint8_t *)data;
    ctx->remaining = content_len;
    ctx->copy_flag = copy_flag;

    http_pump_conn(ctx);
}

static void send_html_page(struct tcp_pcb *tpcb, const char *page) {
    send_raw_data(tpcb, "text/html; charset=utf-8", page, (int)strlen(page));
}

/**
 * @brief Roteador principal e despachante de requisições HTTP do servidor Web.
 * 
 * Processa requisições nos modos Setup (10.10.0.1) e Operacional (2.0.N.1):
 *   - Rota '/' : Entrega a Dashboard operacional ou a página de Setup Wizard.
 *   - Rota '/monitor' : Monitor DMX em tempo real com visualização dinâmica de universos.
 *   - Rota '/tester' : Testador manual de faders e canais DMX individuais.
 *   - Rota '/status' : API REST em formato JSON com telemetria (FPS, PPS, pacotes, uptime).
 *   - Rota '/stream' : Fluxo binário contínuo dos 512 canais DMX a 60 FPS.
 *   - Rota '/img/picow.webp' e '/img/rp2040_zero.webp' : Envio dos diagramas de pinagem.
 *   - Rota POST '/salvar' : Persistência de novas configurações e reinicialização.
 *   - Rota POST '/api/test_channel' e '/api/test_all' : Comandos de teste manual.
 *   - Rota POST '/api/theme' : Alternância rápida entre temas Claro, Escuro e Auto.
 *   - Rota POST '/reset' : Restauração de padrões de fábrica.
 * 
 * @param arg Argumento de contexto.
 * @param tpcb PCB TCP da conexão ativa.
 * @param p Buffer pbuf contendo a requisição HTTP recebida.
 * @param err Código de erro lwIP.
 * @return err_t ERR_OK em sucesso.
 */
static err_t http_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    (void) arg; (void) err;
    if (!p) {
        for (int i = 0; i < HTTP_MAX_CONNS; i++) {
            if (s_http_conns[i].in_use && s_http_conns[i].pcb == tpcb) {
                http_free_conn(&s_http_conns[i]);
                break;
            }
        }
        http_close_pcb(tpcb);
        return ERR_OK;
    }

    char req[512];
    size_t copy_len = (p->tot_len < sizeof(req) - 1) ? p->tot_len : (sizeof(req) - 1);
    pbuf_copy_partial(p, req, copy_len, 0);
    req[copy_len] = '\0';

    // Consome imediatamente os bytes recebidos da pilha lwIP e libera o pbuf.
    // Isso evita qualquer violação de use-after-free e impede que fechamentos
    // subsequentes gerem TCP RST indevido (NS_ERROR_NET_RESET no navegador).
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);

    // Resposta imediata para preflight CORS e Private Network Access (PNA)
    if (strncmp(req, "OPTIONS", 7) == 0) {
        const char *resp = "HTTP/1.1 204 No Content\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Access-Control-Allow-Private-Network: true\r\n"
                           "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                           "Access-Control-Allow-Headers: *\r\n"
                           "Content-Length: 0\r\n"
                           "Connection: close\r\n\r\n";
        tcp_write(tpcb, resp, strlen(resp), TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
        http_close_pcb(tpcb);
        return ERR_OK;
    }

    // Resposta imediata para favicon
    if (strncmp(req, "GET /favicon.ico", 16) == 0) {
        const char *resp = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        tcp_write(tpcb, resp, strlen(resp), TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
        http_close_pcb(tpcb);
        return ERR_OK;
    }

    // Mudança rápida de idioma via AJAX (/set_lang?l=0..7)
    if (strncmp(req, "GET /set_lang", 13) == 0) {
        char *p_l = strstr(req, "l=");
        if (p_l) {
            int l = atoi(p_l + 2);
            if (l >= 0 && l < LANG_COUNT) {
                if (g_config.language != (uint8_t)l) {
                    g_config.language = (uint8_t)l;
                    storage_save();
                    ssd1306_update();
                }
            }
        }
        send_quick_response(tpcb, "application/json", "{\"success\":1}", 13);
        return ERR_OK;
    }

    // Mudança e persistência rápida de tema via AJAX (/set_theme?t=0/1/2) - 0=Auto, 1=Dark, 2=Light
    if (strncmp(req, "GET /set_theme", 14) == 0) {
        char *p_t = strstr(req, "t=");
        if (p_t) {
            int t = atoi(p_t + 2);
            if (t >= 0 && t <= 2) {
                if (g_config.theme != (uint8_t)t) {
                    g_config.theme = (uint8_t)t;
                    storage_save();
                }
            }
        }
        send_quick_response(tpcb, "application/json", "{\"success\":1}", 13);
        return ERR_OK;
    }

    // ISOLAMENTO RIGOROSO DE ROTAS:
    // Em Modo de Configuração Inicial (10.10.0.1 / !g_config.configured), o dispositivo só
    // pode acessar o assistente de configurações. Bloqueia qualquer rota operacional utilizada no IP 2.x.x.x
    // (/monitor, /tester, /events, /status, /dmx_stream, /dmx_raw, etc.) e redireciona para a raiz '/'
    if (!g_config.configured) {
        if (strncmp(req, "GET /monitor", 12) == 0 ||
            strncmp(req, "GET /mon", 8) == 0 ||
            strncmp(req, "GET /tester", 11) == 0 ||
            strncmp(req, "GET /tester_mode", 16) == 0 ||
            strncmp(req, "GET /tester_ping", 16) == 0 ||
            strncmp(req, "GET /test_dmx", 13) == 0 ||
            strncmp(req, "POST /test_dmx", 14) == 0 ||
            strncmp(req, "GET /events", 11) == 0 ||
            strncmp(req, "GET /status", 11) == 0 ||
            strncmp(req, "GET /dmx_stream", 15) == 0 ||
            strncmp(req, "GET /dmx_raw", 12) == 0 ||
            strncmp(req, "GET /set_rate", 13) == 0 ||
            strncmp(req, "GET /set_u", 10) == 0 ||
            strncmp(req, "GET /flip_screen", 16) == 0)
        {
            const char *resp = "HTTP/1.1 303 See Other\r\nLocation: /\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            tcp_write(tpcb, resp, strlen(resp), TCP_WRITE_FLAG_COPY);
            tcp_output(tpcb);
            http_close_pcb(tpcb);
            return ERR_OK;
        }
    }

    // Ferramenta Web de Teste DMX (Channel Fader Tool)
    if (strncmp(req, "GET /test_dmx", 13) == 0 || strncmp(req, "POST /test_dmx", 14) == 0) {
        char temp_u[8] = {0}, temp_ch[8] = {0}, temp_val[8] = {0}, temp_all[8] = {0}, temp_blk[8] = {0}, temp_all_u[8] = {0};
        extract_param(req, "u=", temp_u, sizeof(temp_u));
        extract_param(req, "ch=", temp_ch, sizeof(temp_ch));
        extract_param(req, "val=", temp_val, sizeof(temp_val));
        extract_param(req, "all=", temp_all, sizeof(temp_all));
        extract_param(req, "blackout=", temp_blk, sizeof(temp_blk));
        extract_param(req, "all_univ=", temp_all_u, sizeof(temp_all_u));

        g_manual_test_active = true;
        s_last_manual_test_time = to_ms_since_boot(get_absolute_time());

        if (temp_blk[0] == '1') {
            dmx_blackout_all();
        } else if (temp_all_u[0] != '\0') {
            int all_val = atoi(temp_all_u);
            if (all_val < 0) all_val = 0;
            if (all_val > 255) all_val = 255;
            for (int u = 0; u < MAX_UNIVERSES; u++) {
                dmx_set_all_channels((uint16_t)u, (uint8_t)all_val);
            }
        } else {
            int u = atoi(temp_u);
            if (u < 0) u = 0;
            if (u >= 8) u = 7;

            if (temp_all[0] != '\0') {
                int all_val = atoi(temp_all);
                if (all_val < 0) all_val = 0;
                if (all_val > 255) all_val = 255;
                dmx_set_all_channels((uint16_t)u, (uint8_t)all_val);
            } else if (temp_ch[0] != '\0' && temp_val[0] != '\0') {
                int ch = atoi(temp_ch);
                int val = atoi(temp_val);
                if (ch >= 1 && ch <= 512 && val >= 0 && val <= 255) {
                    dmx_set_channel((uint16_t)u, (uint16_t)ch, (uint8_t)val);
                }
            }
        }

        send_quick_response(tpcb, "application/json", "{\"success\":1}", 13);
        return ERR_OK;
    }

    // Controle e Heartbeat do Modo Teste Manual (/tester_mode e /tester_ping)
    if (strncmp(req, "GET /tester_mode", 16) == 0 || strncmp(req, "GET /tester_ping", 16) == 0) {
        char temp_act[8] = {0};
        extract_param(req, "active=", temp_act, sizeof(temp_act));
        if (temp_act[0] == '0') {
            g_manual_test_active = false;
        } else {
            g_manual_test_active = true;
            s_last_manual_test_time = to_ms_since_boot(get_absolute_time());
        }
        send_quick_response(tpcb, "application/json", "{\"active\":1}", 12);
        return ERR_OK;
    }

    // Página Dedicada do Testador Manual de Faders (/tester)
    if (strncmp(req, "GET /tester", 11) == 0 && (req[11] == ' ' || req[11] == '?' || req[11] == '/' || req[11] == '\r' || req[11] == '\0')) {
        g_manual_test_active = true;
        s_last_manual_test_time = to_ms_since_boot(get_absolute_time());
        send_html_page(tpcb, HTML_TESTER_PAGE);
        return ERR_OK;
    }

    // Stream Server-Sent Events (SSE) - Zero Polling / Zero Handshakes repetidos
    if (strncmp(req, "GET /events", 11) == 0) {
        char *pu = strstr(req, "u=");
        if (pu) {
            int ru = atoi(pu + 2);
            if (ru >= 0 && ru < 8) s_sse_req_u = (uint8_t)ru;
        }

        if (s_sse_pcb && s_sse_pcb != tpcb) {
            http_close_pcb(s_sse_pcb);
            s_sse_pcb = NULL;
        }

        s_sse_pcb = tpcb;
        tcp_arg(tpcb, NULL);
        tcp_sent(tpcb, NULL);
        tcp_err(tpcb, http_sse_err_callback);
        tcp_recv(tpcb, http_sse_recv_callback);

        const char *hdr = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/event-stream\r\n"
                          "Cache-Control: no-cache\r\n"
                          "Access-Control-Allow-Origin: *\r\n"
                          "Access-Control-Allow-Private-Network: true\r\n"
                          "Connection: keep-alive\r\n\r\n";
        tcp_write(tpcb, hdr, strlen(hdr), TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);

        s_last_sse_time = 0;
        http_sse_poll();
        return ERR_OK;
    }

    // Seleção rápida de universo para o stream SSE
    if (strncmp(req, "GET /set_u", 10) == 0) {
        char *pu = strstr(req, "u=");
        if (pu) {
            int ru = atoi(pu + 2);
            if (ru >= 0 && ru < 8) s_sse_req_u = (uint8_t)ru;
        }
        s_last_sse_time = 0;
        http_sse_poll();
        send_quick_response(tpcb, "application/json", "{\"success\":1}", 13);
        return ERR_OK;
    }

    // Telemetria em tempo real (AJAX JSON)
    if (strncmp(req, "GET /status", 11) == 0) {
        int req_u = 0;
        char *pu = strstr(req, "u=");
        if (pu) {
            req_u = atoi(pu + 2);
            if (req_u < 0 || req_u >= 8) req_u = 0;
        }

        const uint8_t *active_buf = dmx_get_active_buffer((uint16_t)req_u);
        uint8_t ch16[16] = {0};
        if (active_buf) {
            memcpy(ch16, &active_buf[1], 16);
        }

        static char s_json_buf[512];
        int jlen = snprintf(s_json_buf, sizeof(s_json_buf),
            "{\"packets\":%lu,\"pps\":%lu,\"fps\":%lu,\"loss\":%lu,\"sync\":%s,"
            "\"univ\":%u,\"sel_u\":%d,\"max_u\":%d,\"start_u\":%u,\"loss_mode\":%u,\"dmx_rate\":%u,"
            "\"node_id\":%u,\"node_name\":\"%s\",\"ip\":\"%s\",\"lang\":%u,\"theme\":%u,\"active\":%s,"
            "\"ch\":[%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u]}",
            g_artnet_packets_total, g_artnet_pps, g_artnet_fps, g_artnet_lost_packets,
            g_artsync_active ? "true" : "false",
            g_artnet_last_universe, req_u, g_config.def_universos,
            g_config.start_universe, (unsigned)g_config.signal_loss_mode, (unsigned)g_config.dmx_rate,
            g_config.node_id, g_config.node_name, s_ip_str,
            (unsigned)g_config.language, (unsigned)g_config.theme,
            g_artnet_active ? "true" : "false",
            ch16[0], ch16[1], ch16[2], ch16[3],
            ch16[4], ch16[5], ch16[6], ch16[7],
            ch16[8], ch16[9], ch16[10], ch16[11],
            ch16[12], ch16[13], ch16[14], ch16[15]);
        send_quick_response(tpcb, "application/json", s_json_buf, jlen);
        return ERR_OK;
    }

    // Monitor Completo dos 8 Universos (4.096 canais)
    if (strncmp(req, "GET /monitor", 12) == 0 || strncmp(req, "GET /mon", 8) == 0) {
        send_html_page(tpcb, HTML_MONITOR_PAGE);
        return ERR_OK;
    }

    // Stream Binário Contínuo de DMX (Zero Polling / Zero Handshakes)
    if (strncmp(req, "GET /dmx_stream", 15) == 0) {
        char *pr = strstr(req, "r=");
        if (pr) {
            int rval = atoi(pr + 2);
            if (rval >= 0 && rval <= 5000) s_dmx_stream_interval_ms = (uint32_t)rval;
        }

        if (s_dmx_stream_pcb && s_dmx_stream_pcb != tpcb) {
            http_close_pcb(s_dmx_stream_pcb);
            s_dmx_stream_pcb = NULL;
        }

        s_dmx_stream_pcb = tpcb;
        tcp_arg(tpcb, NULL);
        tcp_sent(tpcb, NULL);
        tcp_err(tpcb, http_dmx_stream_err_callback);
        tcp_recv(tpcb, http_dmx_stream_recv_callback);

        const char *hdr = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: application/octet-stream\r\n"
                          "Cache-Control: no-cache\r\n"
                          "Access-Control-Allow-Origin: *\r\n"
                          "Access-Control-Allow-Private-Network: true\r\n"
                          "Connection: keep-alive\r\n\r\n";
        tcp_write(tpcb, hdr, strlen(hdr), TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);

        s_last_dmx_stream_time = 0;
        http_dmx_stream_poll();
        return ERR_OK;
    }

    // Ajuste dinâmico de taxa do stream binário
    if (strncmp(req, "GET /set_rate", 13) == 0) {
        char *pr = strstr(req, "r=");
        if (pr) {
            int rval = atoi(pr + 2);
            if (rval >= 0 && rval <= 5000) s_dmx_stream_interval_ms = (uint32_t)rval;
        }
        send_quick_response(tpcb, "application/json", "{\"success\":1}", 13);
        return ERR_OK;
    }

    // Stream Binário de Telemetria e DMX dos Universos (4.112 bytes)
    if (strncmp(req, "GET /dmx_raw", 12) == 0) {
        static uint8_t raw_payload[16 + MAX_UNIVERSES * UNIVERSE_CHANNELS];
        memcpy(&raw_payload[0], (const void *)&g_artnet_packets_total, 4);
        memcpy(&raw_payload[4], (const void *)&g_artnet_pps, 4);
        uint16_t fps16 = (uint16_t)g_artnet_fps;
        memcpy(&raw_payload[8], &fps16, 2);
        raw_payload[10] = (uint8_t)g_config.def_universos; // Universos configurados
        raw_payload[11] = g_config.language; // Idioma atual do sistema
        raw_payload[12] = 'D';
        raw_payload[13] = 'M';
        raw_payload[14] = 'X';
        raw_payload[15] = '8';

        for (int u = 0; u < MAX_UNIVERSES; u++) {
            const uint8_t *abuf = dmx_get_active_buffer((uint16_t)u);
            if (abuf) {
                memcpy(&raw_payload[16 + u * UNIVERSE_CHANNELS], &abuf[1], UNIVERSE_CHANNELS);
            } else {
                memset(&raw_payload[16 + u * UNIVERSE_CHANNELS], 0, UNIVERSE_CHANNELS);
            }
        }
        send_raw_data(tpcb, "application/octet-stream", raw_payload, sizeof(raw_payload));
        return ERR_OK;
    }

    // Imagem do Pinout do Raspberry Pi Pico W (embarcada na Flash ROM)
    if (strncmp(req, "GET /img/picow.webp", 19) == 0 || strncmp(req, "GET /img/picow.png", 18) == 0) {
        send_raw_data(tpcb, "image/webp", g_img_picow_data, (int)g_img_picow_len);
        return ERR_OK;
    }

    // Imagem do Pinout da Waveshare RP2040-Zero (embarcada na Flash ROM)
    if (strncmp(req, "GET /img/rp2040_zero.webp", 25) == 0 || strncmp(req, "GET /img/rp2040_zero.png", 24) == 0) {
        send_raw_data(tpcb, "image/webp", g_img_zero_data, (int)g_img_zero_len);
        return ERR_OK;
    }

    // Reinicia o microcontrolador via Watchdog quando solicitado
    if (strncmp(req, "GET /reboot", 11) == 0) {
        send_html_page(tpcb, HTML_REBOOT_PAGE);
        rede_reboot_system();
        return ERR_OK;
    }

    // Restauração para Padrões de Fábrica via interface web
    if (strncmp(req, "GET /reset_factory", 18) == 0) {
        storage_factory_reset();
        send_factory_reset_page(tpcb);
        rede_reboot_system();
        return ERR_OK;
    }

    // Inverte a rotação da tela OLED (180 graus) em tempo de execução
    if (strncmp(req, "GET /flip_screen", 16) == 0) {
        g_config.rotate180 = !g_config.rotate180;
        storage_save();
        ssd1306_set_rotation(g_config.rotate180 != 0);
        ssd1306_display_on(true);
        g_artnet_last_packet_time = to_ms_since_boot(get_absolute_time());
        ssd1306_update();
        const char *resp = "HTTP/1.1 303 See Other\r\nLocation: /\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        tcp_write(tpcb, resp, strlen(resp), TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
        http_close_pcb(tpcb);
        return ERR_OK;
    }

    // Rota direta para formulário de configuração (/config) - APENAS acessível em modo setup inicial (10.10.0.1)
    if (strncmp(req, "GET /config", 11) == 0) {
        if (!g_config.configured) {
            send_html_page(tpcb, HTML_SETUP_PAGE);
        } else {
            // Em modo operacional (2.0.x.x), bloqueia acesso à rota config e redireciona para a raiz '/'
            const char *resp = "HTTP/1.1 303 See Other\r\nLocation: /\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            tcp_write(tpcb, resp, strlen(resp), TCP_WRITE_FLAG_COPY);
            tcp_output(tpcb);
            http_close_pcb(tpcb);
        }
        return ERR_OK;
    }

    // Salvar configurações de Identidade, Idioma, Tema e Universos (POST /salvar ou GET /salvar)
    if (strncmp(req, "POST /salvar", 12) == 0 || strncmp(req, "GET /salvar", 11) == 0) {
        if (g_config.configured) {
            // Em modo operacional (2.0.x.x), não permite reconfiguração direta; redireciona para a raiz
            const char *resp = "HTTP/1.1 303 See Other\r\nLocation: /\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            tcp_write(tpcb, resp, strlen(resp), TCP_WRITE_FLAG_COPY);
            tcp_output(tpcb);
            http_close_pcb(tpcb);
            return ERR_OK;
        }

        char temp_name[32] = {0};
        char temp_id[16] = {0};
        char temp_lang[8] = {0};
        char temp_loss[8] = {0};
        char temp_rate[8] = {0};
        char temp_univ[8] = {0};
        char temp_theme[8] = {0};

        extract_param(req, "node_name=", temp_name, sizeof(temp_name));
        extract_param(req, "node_id=", temp_id, sizeof(temp_id));
        extract_param(req, "lang=", temp_lang, sizeof(temp_lang));
        extract_param(req, "loss_mode=", temp_loss, sizeof(temp_loss));
        extract_param(req, "dmx_rate=", temp_rate, sizeof(temp_rate));
        extract_param(req, "universos=", temp_univ, sizeof(temp_univ));
        extract_param(req, "theme=", temp_theme, sizeof(temp_theme));

        int id = atoi(temp_id);
        if (id < 1 || id > 16) id = 1;

        int num_u = 8;
        if (temp_univ[0] != '\0') {
            num_u = atoi(temp_univ);
            if (num_u < 1 || num_u > 8) num_u = 8;
        }

        if (temp_lang[0] != '\0') {
            int l = atoi(temp_lang);
            if (l >= 0 && l < LANG_COUNT) {
                g_config.language = (uint8_t)l;
            }
        }

        if (temp_loss[0] != '\0') {
            int lm = atoi(temp_loss);
            if (lm >= 0 && lm <= 1) {
                g_config.signal_loss_mode = (uint8_t)lm;
            }
        }

        if (temp_rate[0] != '\0') {
            int dr = atoi(temp_rate);
            if (dr >= 0 && dr <= 2) {
                g_config.dmx_rate = (uint8_t)dr;
            }
        }

        if (temp_theme[0] != '\0') {
            int th = atoi(temp_theme);
            if (th >= 0 && th <= 2) {
                g_config.theme = (uint8_t)th;
            }
        }

        if (temp_name[0] != '\0') {
            strncpy(g_config.node_name, temp_name, sizeof(g_config.node_name) - 1);
            g_config.node_name[sizeof(g_config.node_name) - 1] = '\0';
        } else {
            snprintf(g_config.node_name, sizeof(g_config.node_name), "Pico ArtNet No %d", id);
        }

        g_config.node_id = (uint8_t)id;
        g_config.def_universos = num_u;
        g_config.start_universe = (id - 1) * num_u;
        g_config.configured = 1;

        storage_save();

        send_save_page(tpcb, (uint8_t)id);
        rede_reboot_system();
        return ERR_OK;
    }

    // Rota padrão '/':
    // Se o dispositivo ainda não foi configurado, exibe diretamente o assistente de configuração (Setup)
    // Se já estiver configurado, exibe o painel de monitoramento ao vivo (Dashboard)
    if (strncmp(req, "GET ", 4) == 0) {
        if (!g_config.configured) {
            send_html_page(tpcb, HTML_SETUP_PAGE);
        } else {
            send_html_page(tpcb, HTML_DASHBOARD_PAGE);
        }
        return ERR_OK;
    }

    http_close_pcb(tpcb);
    return ERR_OK;
}

/**
 * @brief Callback de aceitação de novas conexões TCP na porta 80 do servidor HTTP.
 * 
 * @param arg Argumento reservado.
 * @param newpcb Novo PCB TCP criado para a conexão do cliente.
 * @param err Código de status lwIP.
 * @return err_t ERR_OK se aceito com sucesso.
 */
static err_t http_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    (void) arg; (void) err;
    tcp_recv(newpcb, http_recv_callback);
    return ERR_OK;
}

/**
 * @brief Inicializa o servidor Web HTTP na porta TCP 80.
 * 
 * Cria uma PCB TCP do lwIP, associa à porta 80 e ativa o callback http_accept_callback.
 */
void http_server_start(void) {
    s_http_pcb = tcp_new();
    if (s_http_pcb) {
        tcp_bind(s_http_pcb, IP_ADDR_ANY, HTTP_PORT);
        s_http_pcb = tcp_listen(s_http_pcb);
        tcp_accept(s_http_pcb, http_accept_callback);
        printf("[HTTP] Servidor Web iniciado na porta %d\n", HTTP_PORT);
    }
}

/**
 * @brief Inicializa a escuta de pacotes UDP na porta 6454 do protocolo Art-Net.
 * 
 * Cria uma PCB UDP do lwIP, associa à porta 6454 e registra o callback artnet_udp_recv_callback.
 */
void artnet_start_listener(void) {
    s_artnet_pcb = udp_new();
    if (s_artnet_pcb) {
        udp_bind(s_artnet_pcb, IP_ADDR_ANY, ARTNET_PORT);
        udp_recv(s_artnet_pcb, artnet_udp_recv_callback, NULL);
        printf("[ARTNET] Receptor UDP iniciado na porta %d\n", ARTNET_PORT);
    }
}

/**
 * @brief Inicialização global de todos os módulos e serviços da camada de rede.
 * 
 * Configura parâmetros de rede IP, inicializa o driver USB Ethernet, registra a interface
 * netif do lwIP, ativa o servidor DHCP, o servidor DNS (se em setup), o receptor Art-Net
 * e o servidor Web HTTP.
 */
void rede_init(void) {
    printf("============================================================\n");
    printf("  [REDE] INICIALIZANDO INTERFACE DE REDE USB ETHERNET\n");
    if (!g_config.configured) {
        printf("  -> MODO SETUP: 10.10.0.1 | DHCP para o PC: 10.10.0.2\n");
    } else {
        printf("  -> NO %02u | IP: 2.0.%u.1 | DHCP para o PC: 2.0.%u.2\n",
               g_config.node_id, g_config.node_id, g_config.node_id);
        printf("  -> UNIVERSOS DMX ATIVOS: %u a %u (8 Saidas)\n",
               g_config.start_universe, g_config.start_universe + 7);
    }
    printf("============================================================\n");

    s_usb_mode = true;
    g_artnet_last_packet_time = to_ms_since_boot(get_absolute_time());
    lwip_init();
    init_usb_ethernet();
}

/**
 * @brief Rotina de polling contínuo da rede (chamada repetidamente no loop do Core 0).
 * 
 * Processa eventos USB através de `tud_task()`, atende o tráfego da fila de recepção (`service_traffic()`),
 * gerencia timeouts da pilha lwIP (`sys_check_timeouts()`), e bombeia conexões HTTP e streaming DMX.
 */
void rede_poll(void) {
    tud_task();
    service_traffic();
    http_sse_poll();
    http_dmx_stream_poll();
    http_poll_conns();
}

/**
 * @brief Informa se o link de rede USB está ativo e conectado ao computador host.
 * @return true se conectado; false caso contrário.
 */
bool rede_is_connected(void) {
    return tud_ready();
}

/**
 * @brief Retorna uma string contendo o endereço IP atual do nó (ex: "10.10.0.1" ou "2.0.1.1").
 * @return const char* Ponteiro para a string formatada do endereço IP.
 */
const char *rede_get_ip_str(void) {
    return s_ip_str;
}
