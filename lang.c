/**
 * ============================================================================
 * PROJETO: PICO ARTNET DMX NODE (RP2040)
 * ARQUIVO: lang.c
 * AUTOR: Matheus Dev (@Matheus-devpixel)
 * DESCRIÇÃO: Implementação das tabelas de internacionalização (i18n) contendo
 *            as traduções dos textos para a tela OLED SSD1306 e para as páginas
 *            web de transição do sistema nos 8 idiomas suportados.
 * ============================================================================
 */

#include "lang.h"

const oled_lang_strings_t g_oled_lang[LANG_COUNT] = {
    [LANG_PT] = {
        .setup_title        = "PICO - CONFIG",
        .setup_browser      = "Acesse no Navegador:",
        .setup_waiting      = "AGUARDANDO CONFIG",
        .status_node        = "PICO - %d",
        .status_univ        = "DMX: %u-%u (%d Uni.)",
        .status_port        = "Porta: 6454",
        .status_idle        = "ArtNet: SEM DADOS",
        .reset_title        = "CONFIRME O RESET",
        .reset_prompt       = "SEGURE P/ CONFIRMAR",
        .reset_confirmed_t  = "RESET DE FABRICA",
        .reset_confirmed_m  = "RESTAURADO!",
        .cancel_title       = "CANCELADO",
        .cancel_msg         = "Reset cancelado",
        .starting_msg       = "Iniciando..."
    },
    [LANG_EN] = {
        .setup_title        = "PICO - SETUP",
        .setup_browser      = "Open in Browser:",
        .setup_waiting      = "WAITING CONFIG",
        .status_node        = "PICO - %d",
        .status_univ        = "DMX: %u-%u (%d Uni.)",
        .status_port        = "Port: 6454",
        .status_idle        = "ArtNet: NO DATA",
        .reset_title        = "CONFIRM RESET",
        .reset_prompt       = "HOLD TO CONFIRM",
        .reset_confirmed_t  = "FACTORY RESET",
        .reset_confirmed_m  = "RESTORED!",
        .cancel_title       = "CANCELLED",
        .cancel_msg         = "Reset cancelled",
        .starting_msg       = "Starting..."
    },
    [LANG_ES] = {
        .setup_title        = "PICO - AJUSTES",
        .setup_browser      = "Abrir en Navegador:",
        .setup_waiting      = "ESPERANDO CONFIG",
        .status_node        = "PICO - %d",
        .status_univ        = "DMX: %u-%u (%d Uni.)",
        .status_port        = "Puerto: 6454",
        .status_idle        = "ArtNet: SIN DATOS",
        .reset_title        = "CONFIRME RESET",
        .reset_prompt       = "MANTENGA PRESIONADO",
        .reset_confirmed_t  = "RESET DE FABRICA",
        .reset_confirmed_m  = "RESTAURADO!",
        .cancel_title       = "CANCELADO",
        .cancel_msg         = "Reset cancelado",
        .starting_msg       = "Iniciando..."
    },
    [LANG_FR] = {
        .setup_title        = "PICO - CONFIG",
        .setup_browser      = "Ouvrir Navigateur:",
        .setup_waiting      = "ATTENTE CONFIG",
        .status_node        = "PICO - %d",
        .status_univ        = "DMX: %u-%u (%d Uni.)",
        .status_port        = "Port: 6454",
        .status_idle        = "ArtNet: PAS DE SIGNAL",
        .reset_title        = "CONFIRMER RESET",
        .reset_prompt       = "MAINTENIR P/ VALIDER",
        .reset_confirmed_t  = "RESET D'USINE",
        .reset_confirmed_m  = "RESTAURE!",
        .cancel_title       = "ANNULE",
        .cancel_msg         = "Reset annule",
        .starting_msg       = "Demarrage..."
    },
    [LANG_DE] = {
        .setup_title        = "PICO - EINSTELL",
        .setup_browser      = "Im Browser oeffnen:",
        .setup_waiting      = "WARTE AUF CONFIG",
        .status_node        = "PICO - %d",
        .status_univ        = "DMX: %u-%u (%d Uni.)",
        .status_port        = "Port: 6454",
        .status_idle        = "ArtNet: KEINE DATEN",
        .reset_title        = "RESET BESTAETIGEN",
        .reset_prompt       = "HALTEN ZUM RESET",
        .reset_confirmed_t  = "WERKSEINSTELLUNG",
        .reset_confirmed_m  = "ZURUECKGESETZT!",
        .cancel_title       = "ABGEBROCHEN",
        .cancel_msg         = "Reset abgebrochen",
        .starting_msg       = "Starten..."
    },
    [LANG_IT] = {
        .setup_title        = "PICO - CONFIG",
        .setup_browser      = "Apri nel Browser:",
        .setup_waiting      = "ATTESA CONFIG",
        .status_node        = "PICO - %d",
        .status_univ        = "DMX: %u-%u (%d Uni.)",
        .status_port        = "Porta: 6454",
        .status_idle        = "ArtNet: NESSUN DATO",
        .reset_title        = "CONFERMA RESET",
        .reset_prompt       = "TIENI P/ CONFERMA",
        .reset_confirmed_t  = "RESET FABBRICA",
        .reset_confirmed_m  = "RIPRISTINATO!",
        .cancel_title       = "ANNULLATO",
        .cancel_msg         = "Reset annullato",
        .starting_msg       = "Avvio..."
    },
    [LANG_RU] = {
        .setup_title        = "PICO - SETUP",
        .setup_browser      = "Otkryt v Brauzere:",
        .setup_waiting      = "OZHIDANIE CONFIG",
        .status_node        = "PICO - %d",
        .status_univ        = "DMX: %u-%u (%d Uni.)",
        .status_port        = "Port: 6454",
        .status_idle        = "ArtNet: NET DANNYH",
        .reset_title        = "PODTVERDITE SBROS",
        .reset_prompt       = "UDERZHIVAYTE KNOPKU",
        .reset_confirmed_t  = "ZAVODSKOY SBROS",
        .reset_confirmed_m  = "VOSSTANOVLENO!",
        .cancel_title       = "OTMENENO",
        .cancel_msg         = "Sbros otmenen",
        .starting_msg       = "Zapusk..."
    },
    [LANG_ZH] = {
        .setup_title        = "PICO - SETUP",
        .setup_browser      = "Open in Browser:",
        .setup_waiting      = "WAITING CONFIG",
        .status_node        = "PICO - %d",
        .status_univ        = "DMX: %u-%u (%d Uni.)",
        .status_port        = "Port: 6454",
        .status_idle        = "ArtNet: NO DATA",
        .reset_title        = "CONFIRM RESET",
        .reset_prompt       = "HOLD TO CONFIRM",
        .reset_confirmed_t  = "FACTORY RESET",
        .reset_confirmed_m  = "RESTORED!",
        .cancel_title       = "CANCELLED",
        .cancel_msg         = "Reset cancelled",
        .starting_msg       = "Starting..."
    }
};

const web_reboot_strings_t g_web_reboot_lang[LANG_COUNT] = {
    [LANG_PT] = {
        .save_title    = "Configurações Salvas na Flash!",
        .save_msg      = "O nó Pico ArtNet foi configurado com sucesso como <b>Nó %02u</b> (Universos %u a %u).",
        .reboot_wait   = "O microcontrolador está reiniciando para aplicar o novo endereço de rede IP.",
        .redirecting   = "Redirecionando em <span id='cnt' style='font-weight:700; color:#0284c7;'>10</span> segundos...",
        .btn_manual    = "Acessar Diretamente",
        .reset_title   = "Padrões de Fábrica Restaurados",
        .reset_msg     = "O nó foi redefinido e retornou ao modo de assistente de configuração inicial.",
        .reboot_title  = "Reiniciando Microcontrolador...",
        .reboot_msg    = "O nó Pico ArtNet está reiniciando."
    },
    [LANG_EN] = {
        .save_title    = "Settings Saved to Flash!",
        .save_msg      = "Pico ArtNet node successfully configured as <b>Node %02u</b> (Universes %u to %u).",
        .reboot_wait   = "The microcontroller is rebooting to apply the new IP network address.",
        .redirecting   = "Redirecting in <span id='cnt' style='font-weight:700; color:#0284c7;'>10</span> seconds...",
        .btn_manual    = "Access Directly",
        .reset_title   = "Factory Defaults Restored",
        .reset_msg     = "The node has been reset and returned to initial setup wizard mode.",
        .reboot_title  = "Rebooting Microcontroller...",
        .reboot_msg    = "The Pico ArtNet node is rebooting."
    },
    [LANG_ES] = {
        .save_title    = "¡Configuración Guardada en Flash!",
        .save_msg      = "El nodo Pico ArtNet fue configurado con éxito como <b>Nodo %02u</b> (Universos %u a %u).",
        .reboot_wait   = "El microcontrolador se está reiniciando para aplicar la nueva dirección IP.",
        .redirecting   = "Redirigiendo en <span id='cnt' style='font-weight:700; color:#0284c7;'>10</span> segundos...",
        .btn_manual    = "Acceder Directamente",
        .reset_title   = "Valores de Fábrica Restablecidos",
        .reset_msg     = "El nodo se restableció y regresó al modo de asistente de configuración inicial.",
        .reboot_title  = "Reiniciando Microcontrolador...",
        .reboot_msg    = "El nodo Pico ArtNet se está reiniciando."
    },
    [LANG_FR] = {
        .save_title    = "Paramètres enregistrés dans la Flash !",
        .save_msg      = "Nœud Pico ArtNet configuré avec succès en tant que <b>Nœud %02u</b> (Univers %u à %u).",
        .reboot_wait   = "Le microcontrôleur redémarre pour appliquer la nouvelle adresse réseau IP.",
        .redirecting   = "Redirection dans <span id='cnt' style='font-weight:700; color:#0284c7;'>10</span> secondes...",
        .btn_manual    = "Accéder Directement",
        .reset_title   = "Paramètres d'usine restaurés",
        .reset_msg     = "Le nœud a été réinitialisé et est revenu au mode assistant de configuration.",
        .reboot_title  = "Redémarrage du microcontrôleur...",
        .reboot_msg    = "Le nœud Pico ArtNet redémarre."
    },
    [LANG_DE] = {
        .save_title    = "Einstellungen im Flash gespeichert!",
        .save_msg      = "Pico ArtNet Knoten erfolgreich als <b>Knoten %02u</b> (Universen %u bis %u) konfiguriert.",
        .reboot_wait   = "Der Mikrocontroller startet neu, um die neue IP-Netzwerkadresse zu übernehmen.",
        .redirecting   = "Weiterleitung in <span id='cnt' style='font-weight:700; color:#0284c7;'>10</span> Sekunden...",
        .btn_manual    = "Direkt öffnen",
        .reset_title   = "Werkseinstellungen wiederhergestellt",
        .reset_msg     = "Der Knoten wurde zurückgesetzt und befindet sich wieder im Einrichtungsmodus.",
        .reboot_title  = "Mikrocontroller wird neu gestartet...",
        .reboot_msg    = "Der Pico ArtNet Knoten startet neu."
    },
    [LANG_IT] = {
        .save_title    = "Impostazioni salvate nella Flash!",
        .save_msg      = "Nodo Pico ArtNet configurato con successo come <b>Nodo %02u</b> (Universi da %u a %u).",
        .reboot_wait   = "Il microcontrollore si sta riavviando per applicare il nuovo indirizzo IP di rete.",
        .redirecting   = "Reindirizzamento tra <span id='cnt' style='font-weight:700; color:#0284c7;'>10</span> secondi...",
        .btn_manual    = "Accedi Direttamente",
        .reset_title   = "Impostazioni di fabbrica ripristinate",
        .reset_msg     = "Il nodo è stato reimpostato ed è tornato alla modalità di configurazione iniziale.",
        .reboot_title  = "Riavvio del microcontrollore...",
        .reboot_msg    = "Il nodo Pico ArtNet si sta riavviando."
    },
    [LANG_RU] = {
        .save_title    = "Настройки сохранены во Flash!",
        .save_msg      = "Узел Pico ArtNet успешно настроен как <b>Узел %02u</b> (Юниверсы от %u до %u).",
        .reboot_wait   = "Микроконтроллер перезагружается для применения нового IP-адреса.",
        .redirecting   = "Перенаправление через <span id='cnt' style='font-weight:700; color:#0284c7;'>10</span> секунд...",
        .btn_manual    = "Перейти прямо сейчас",
        .reset_title   = "Заводские настройки восстановлены",
        .reset_msg     = "Узел сброшен и возвращен в режим первоначальной настройки.",
        .reboot_title  = "Перезагрузка микроконтроллера...",
        .reboot_msg    = "Узел Pico ArtNet перезагружается."
    },
    [LANG_ZH] = {
        .save_title    = "配置已保存至 Flash！",
        .save_msg      = "Pico ArtNet 节点已成功配置为 <b>节点 %02u</b>（Universe %u 至 %u）。",
        .reboot_wait   = "微控制器正在重启以应用新的 IP 网络地址。",
        .redirecting   = "<span id='cnt' style='font-weight:700; color:#0284c7;'>10</span> 秒后自动跳转...",
        .btn_manual    = "直接访问",
        .reset_title   = "已恢复出厂设置",
        .reset_msg     = "节点已重置并返回至初始配置向导模式。",
        .reboot_title  = "正在重启微控制器...",
        .reboot_msg    = "Pico ArtNet 节点正在重启。"
    }
};
