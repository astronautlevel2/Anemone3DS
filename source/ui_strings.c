/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2024 Contributors in CONTRIBUTORS.md
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
*       * Requiring preservation of specified reasonable legal notices or
*         author attributions in that material or in the Appropriate Legal
*         Notices displayed by works containing it.
*       * Prohibiting misrepresentation of the origin of that material,
*         or requiring that modified versions of such material be marked in
*         reasonable ways as different from the original version.
*/

#include "ui_strings.h"
#include "fs.h"

const Language_s language_english = {
    .normal_instructions = 
    {
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Install Theme(s)",
                    "\uE001 Queue shuffle theme"
                },
                {
                    "\uE002 More options",
                    "\uE003 Preview theme"
                },
                {
                    "\uE004 Switch to splashes",
                    "\uE005 Scan QR code"
                },
                {
                    "Exit",
                    "Delete from SD"
                }
            }
        },
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Install splash",
                    "\uE001 Delete installed splash"
                },
                {
                    "\uE002 More options",
                    "\uE003 Preview splash"
                },
                {
                    "\uE004 Switch to themes",
                    "\uE005 Scan QR code"
                },
                {
                    "Exit",
                    "Delete from SD"
                }
            }
        }
    },

    .install_instructions =
    {
        .info_line = "\uE001 Cancel theme install",
        .instructions = {
            {
                "\uE079 Normal install",
                "\uE07A Shuffle install"
            },
            {
                "\uE07B BGM-only install",
                "\uE07C No-BGM install"
            },
            {
                NULL,
                NULL
            },
            {
                "Exit",
                NULL
            }
        }
    },

    .extra_instructions = 
    {
        {
            .info_line = "\uE001 Leave sorting menu",
            .instructions = {
                {
                    "\uE079 Sort by name",
                    "\uE07A Sort by author"
                },
                {
                    "\uE07B Sort by filename",
                    NULL
                },
                {
                    NULL,
                    NULL
                },
                {
                    "Exit",
                    NULL
                }
            }
        },
        {
            .info_line = "\uE001 Leave extra menu",
            .instructions = {
                {
                    "\uE079 Jump in the list",
                    "\uE07A Reload broken icons"
                },
                {
                    "\uE07B Browse ThemePlaza",
                    "\uE07C Install Badges"
                },
                {
                    "\uE004 Sorting menu",
                    "\uE005 Dumping menu"
                },
                {
                    "Exit",
                    NULL
                }
            }
        },
        {
            .info_line = "\uE001 Leave dump menu",
            .instructions = {
                {
                    "\uE079 Dump Current Theme",
                    "\uE07A Dump All Themes"
                },
                {
                    "\uE07B Dump Badges",
                    NULL
                },
                {
                    NULL,
                    NULL
                },
                {
                    "Exit",
                    NULL
                }
            }
        }
    },
    .camera = 
    {
        .quit = "Press \uE005 To Quit",
        .thread_error = "Capture cam thread creation failed\nPlease report this to the developers",
        .zip_not_theme_splash = "Zip downloaded is neither\na splash nor a theme",
        .file_not_zip = "File downloaded isn't a zip.",
        .download_failed = "Download failed.",
        .badge_question = "Theme or splash not detected.\nIs this a badge?",
    },
    .draw = 
    {
        .theme_mode = "Theme mode",
        .splash_mode = "Splash mode",
        .no_themes = "No theme found",
        .no_splashes = "No splash found",
        .qr_download = "Press \uE005 to download from QR",
        .switch_splashes = "Or \uE004 to switch to splashes",
        .switch_themes = "Or \uE004 to switch to themes",
        .quit = "Or        to quit",
        .start_pos = 162, // Adjust x pos of start glyph to line up with quit string
        .by = "By ",
        .selected = "Selected:",
        .sel = "Sel.:",
        .tp_theme_mode = "ThemePlaza Theme mode",
        .tp_splash_mode = "ThemePlaza Splash mode",
        .tp_badge_mode = "ThemePlaza Badge mode",
        .search = "Search...",
        .page = "Page:",
        .err_quit = "Press \uE000 to quit.",
        .warn_continue = "Press \uE000 to continue.",
        .yes_no = "\uE000 Yes   \uE001 No",
        .load_themes = "Loading themes, please wait...",
        .load_splash = "Loading splashes, please wait...",
        .load_icons = "Loading icons, please wait...",
        .install_splash = "Installing a splash...",
        .delete_splash = "Deleting installed splash...",
        .install_theme = "Installing a single theme...",
        .install_shuffle = "Installing shuffle themes...",
        .install_bgm = "Installing BGM-only theme...",
        .install_no_bgm = "Installing theme without BGM...",
        .downloading = "Downloading...",
        .checking_dl = "Checking downloaded file...",
        .delete_sd = "Deleting from SD...",
        .download_themes = "Downloading theme list, please wait...",
        .download_splashes = "Downloading splash list, please wait...",
        .download_badges = "Downloading badge list, please wait...",
        .download_preview = "Downloading preview, please wait...",
        .download_bgm = "Downloading BGM, please wait...",
        .dump_single = "Dumping theme, please wait...",
        .dump_all_official = "Dumping official themes, please wait...",
        .dump_badges = "Dumping installed badges, please wait...",
        .install_badges = "Installing badges, please wait...",
        .shuffle = "Shuffle: %i/10",
    },
    .fs =
    {
        .illegal_input = "Input must not contain:\n" ILLEGAL_CHARS,
        .new_or_overwrite = "Choose a new filename or tap Overwrite",
        .cancel = "Cancel",
        .overwrite = "Overwrite",
        .rename = "Rename",
        .swkbd_fail = "???\nTry a USB keyboard", // Should never be used
        .sd_full = "SD card is full.\nDelete some themes to make space.",
        .fs_error = "Error:\nGet a new SD card.",
    },
    .loading =
    {
        .no_preview = "No preview found.",
    },
    .main =
    {
        .position_too_big = "The new position has to be\nsmaller or equal to the\nnumber of entries!",
        .position_zero = "The new position has to\nbe positive!",
        .jump_q = "Where do you want to jump to?\nMay cause icons to reload.",
        .cancel = "Cancel",
        .jump = "Jump",
        .no_theme_extdata = "Theme extdata does not exist!\nSet a default theme from the home menu.",
        .loading_qr = "Loading QR Scanner...",
        .no_wifi = "Please connect to Wi-Fi before scanning QR codes",
        .qr_homebrew = "QR scanning doesnt work from the Homebrew\nLauncher, use the ThemePlaza browser instead.",
        .camera_broke = "Your camera seems to have a problem,\nunable to scan QR codes.",
        .too_many_themes = "You have too many themes selected.",
        .not_enough_themes = "You don't have enough themes selected.",
        .uninstall_confirm = "Are you sure you would like to delete\nthe installed splash?",
        .delete_confirm = "Are you sure you would like to delete this?",
    },
    .remote =
    {
        .no_results = "No results for this search.",
        .check_wifi = "Couldn't download Theme Plaza data.\nMake sure WiFi is on.",
        .new_page_big = "The new page has to be\nsmaller or equal to the\nnumber of pages!",
        .new_page_zero = "The new position has to\nbe positive!",
        .jump_page = "Which page do you want to jump to?",
        .cancel = "Cancel",
        .jump = "Jump",
        .tags = "Which tags do you want to search for?",
        .search = "Search",
        .parental_fail = "Parental Control validation failed!\nBrowser Access restricted.",
        .zip_not_found = "ZIP not found at this URL\nIf you believe this is an error, please\ncontact the site administrator",
        .generic_httpc_error = "Error in HTTPC sysmodule - 0x%08lx.\nIf you are seeing this, please contact an\nAnemone developer on the Theme Plaza Discord.",
        .http303_tp = "HTTP 303 See Other (Theme Plaza)\nHas this theme been approved?",
        .http303 = "HTTP 303 See Other\nDownload the resource directly\nor contact the site administrator.",
        .http404 = "HTTP 404 Not Found\nHas this theme been approved?",
        .http_err_url = "HTTP %s\nCheck that the URL is correct.",
        .http_errcode_generic = "HTTP %s\nContact the site administrator.",
        .http401 = "401 Unauthorized",
        .http403 = "403 Forbidden",
        .http407 = "407 Proxy Authentication Required",
        .http414 = "HTTP 414 URI Too Long\nThe QR code points to a really long URL.\nDownload the file directly.",
        .http418 = "HTTP 418 I'm a teapot\nContact the site administrator.",
        .http426 = "HTTP 426 Upgrade Required\nThe 3DS cannot connect to this server.\nContact the site administrator.",
        .http451 = "HTTP 451 Unavailable for Legal Reasons\nSome entity is preventing access\nto the host server for legal reasons.",
        .http500 = "HTTP 500 Internal Server Error\nContact the site administrator.",
        .http502 = "HTTP 502 Bad Gateway\nContact the site administrator.",
        .http503 = "HTTP 503 Service Unavailable\nContact the site administrator.",
        .http504 = "HTTP 504 Gateway Timeout\nContact the site administrator.",
        .http_unexpected = "HTTP %u\nIf you believe this is unexpected, please\ncontact the site administrator.",
    },
    .remote_instructions =
    {
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Download theme",
                    "\uE001 Go back"
                },
                {
                    "\uE002 More options",
                    "\uE003 Preview theme"
                },
                {
                    "\uE004 Previous page",
                    "\uE005 Next page"
                },
                {
                    "Exit",
                    NULL
                }
            }
        },
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Download splash",
                    "\uE001 Go back"
                },
                {
                    "\uE002 More options",
                    "\uE003 Preview splash"
                },
                {
                    "\uE004 Previous page",
                    "\uE005 Next page"
                },
                {
                    "Exit",
                    NULL
                }
            }
        },
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Download badges",
                    "\uE001 Go back"
                },
                {
                    "\uE002 More options",
                    "\uE003 Preview badges"
                },
                {
                    "\uE004 Previous page",
                    "\uE005 Next page"
                },
                {
                    "Exit",
                    NULL
                }
            }
        }
    },
    .remote_extra_instructions =
    {
        {
            .info_line = "\uE001 Leave extra menu",
            .instructions = {
                {
                    "\uE079 Jump to page",
                    "\uE07A Search tags"
                },
                {
                    NULL,
                    "\uE07C Reload without cache"
                },
                {
                    "\uE004 Switch to Badges",
                    "\uE005 Switch to Splashes"
                },
                {
                    "Exit",
                    NULL
                }
            }
        },
        {
            .info_line = "\uE001 Leave extra menu",
            .instructions = {
                {
                    "\uE079 Jump to page",
                    "\uE07A Search tags"
                },
                {
                    NULL,
                    "\uE07C Reload without cache"
                },
                {
                    "\uE004 Switch to Themes",
                    "\uE005 Switch to Badges"
                },
                {
                    "Exit",
                    NULL
                }
            }
        },
        {
            .info_line = "\uE001 Leave extra menu",
            .instructions = {
                {
                    "\uE079 Jump to page",
                    "\uE07A Search tags"
                },
                {
                    NULL,
                    "\uE07C Reload without cache"
                },
                {
                    "\uE004 Switch to Splashes",
                    "\uE005 Switch to Themes"
                },
                {
                    "Exit",
                    NULL
                }
            }
        }
    },
    .splashes =
    {
        .no_splash_found = "No splash.bin or splashbottom.bin found.\nIs this a splash?",
        .splash_disabled = "WARNING: Splashes are disabled in Luma Config",
    },
    .themes =
    {
        .no_body_found = "No body_LZ.bin found - is this a theme?",
        .mono_warn = "One or more installed themes use mono audio.\nMono audio causes a number of issues.\nCheck the wiki for more information.",
        .illegal_char = "Illegal character used.",
        .name_folder = "Name of output folder",
        .cancel = "Cancel",
        .done = "Done"
    },
    .badges = 
    {
        .extdata_locked = "Ext Data Locked\nTry pressing the Home Button and then returning\nto Anemone3DS, or using the CIA version instead.\nDebug: 0x%08lx"
    }
};

const Language_s language_spanish = {
    .normal_instructions =
    {
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Instalar tema(s)",
                    "\uE001 Cola de temas aleatorios"
                },
                {
                    "\uE002 Más opciones",
                    "\uE003 Vista previa del tema"
                },
                {
                    "\uE004 Cambiar a fondos",
                    "\uE005 Escanear código QR"
                },
                {
                    "Salir",
                    "Eliminar de la SD"
                }
            }
        },
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Instalar fondo",
                    "\uE001 Eliminar fondo instalado"
                },
                {
                    "\uE002 Más opciones",
                    "\uE003 Vista previa del fondo"
                },
                {
                    "\uE004 Cambiar a temas",
                    "\uE005 Escanear código QR"
                },
                {
                    "Salir",
                    "Eliminar de la SD"
                }
            }
        }
    },

    .install_instructions =
    {
        .info_line = "\uE001 Cancelar instalación del tema",
        .instructions = {
            {
                "\uE079 Instalación normal",
                "\uE07A Instalación aleatoria"
            },
            {
                "\uE07B Instalación solo BGM",
                "\uE07C Instalación sin BGM"
            },
            {
                NULL,
                NULL
            },
            {
                "Salir",
                NULL
            }
        }
    },

    .extra_instructions =
    {
        {
            .info_line = "\uE001 Dejar menú de clasificación",
            .instructions = {
                {
                    "\uE079 Ordenar por nombre",
                    "\uE07A Ordenar por autor"
                },
                {
                    "\uE07B Ordenar por nombre de archivo",
                    NULL
                },
                {
                    NULL,
                    NULL
                },
                {
                    "Salir",
                    NULL
                }
            }
        },
        {
            .info_line = "\uE001 Dejar menú extra",
            .instructions = {
                {
                    "\uE079 Saltar en la lista",
                    "\uE07A Recargar iconos rotos"
                },
                {
                    "\uE07B Explorar ThemePlaza",
                    "\uE07C Instalar Insignias"
                },
                {
                    "\uE004 Menú de clasificación",
                    "\uE005 Menú de volcado"
                },
                {
                    "Salir",
                    NULL
                }
            }
        },
        {
            .info_line = "\uE001 Dejar menú de volcado",
            .instructions = {
                {
                    "\uE079 Volcar tema actual",
                    "\uE07A Volcar todos los temas"
                },
                {
                    "\uE07B Volcar Insignias",
                    NULL
                },
                {
                    NULL,
                    NULL
                },
                {
                    "Salir",
                    NULL
                }
            }
        }
    },
    .camera =
    {
        .quit = "Presiona \uE005 para salir",
        .thread_error = "Error al crear hilo de cámara\nPor favor, informa a los desarrolladores",
        .zip_not_theme_splash = "El archivo ZIP descargado no es\nni un fondo ni un tema",
        .file_not_zip = "El archivo descargado no es un ZIP.",
        .download_failed = "Fallo en la descarga.",
        .badge_question = "Tema o splash no detectado \n ¿Es esto una insignia?",
    },
    .draw =
    {
        .theme_mode = "Modo tema",
        .splash_mode = "Modo fondo",
        .no_themes = "No se encontraron temas",
        .no_splashes = "No se encontraron fondos",
        .qr_download = "Presiona \uE005 para descargar desde el código QR",
        .switch_splashes = "O \uE004 para cambiar a fondos",
        .switch_themes = "O \uE004 para cambiar a temas",
        .quit = "O        para salir",
        .start_pos = 162,
        .by = "Por ",
        .selected = "Seleccionado:",
        .sel = "Sel.:",
        .tp_theme_mode = "Modo tema de ThemePlaza",
        .tp_splash_mode = "Modo fondo de ThemePlaza",
        .tp_badge_mode = "ThemePlaza Badge mode",
        .search = "Buscar...",
        .page = "Página:",
        .err_quit = "Presiona \uE000 para salir.",
        .warn_continue = "Presiona \uE000 para continuar.",
        .yes_no = "\uE000 Sí   \uE001 No",
        .load_themes = "Cargando temas, por favor espera...",
        .load_splash = "Cargando fondos, por favor espera...",
        .load_icons = "Cargando iconos, por favor espera...",
        .install_splash = "Instalando fondo...",
        .delete_splash = "Eliminando fondo instalado...",
        .install_theme = "Instalando un solo tema...",
        .install_shuffle = "Instalando temas aleatorios...",
        .install_bgm = "Instalando tema solo BGM...",
        .install_no_bgm = "Instalando tema sin BGM...",
        .downloading = "Descargando...",
        .checking_dl = "Comprobando archivo descargado...",
        .delete_sd = "Eliminando de la SD...",
        .download_themes = "Descargando lista de temas,\npor favor espera...",
        .download_splashes = "Descargando lista de fondos, por favor espera...",
        .download_badges = "Descargando lista de insignias,\npor favor espera...",
        .download_preview = "Descargando vista previa,\npor favor espera...",
        .download_bgm = "Descargando BGM, por favor espera...",
        .dump_single = "Volcando tema, por favor espera...",
        .dump_all_official = "Volcando temas oficiales,\npor favor espera...",
        .dump_badges = "Volcando insignias instaladas,\npor favor espera...",
        .install_badges = "Instalando insignias, por favor espera...",
        .shuffle = "Aleatorio: %i/10",
    },
    .fs =
    {
        .illegal_input = "La entrada no debe contener:\n" ILLEGAL_CHARS,
        .new_or_overwrite = "Elige un nombre de archivo nuevo o toca Sobrescribir",
        .cancel = "Cancelar",
        .overwrite = "Sobrescribir",
        .rename = "Renombrar",
        .swkbd_fail = "???\nIntenta con un teclado USB", // Nunca debería ser usado
        .sd_full = "La tarjeta SD está llena.\nElimina algunos temas para liberar espacio.",
        .fs_error = "Error:\nConsigue una nueva tarjeta SD.",
    },
    .loading =
    {
        .no_preview = "No se encontró vista previa.",
    },
    .main =
    {
        .position_too_big = "La nueva posición debe ser\nmenor o igual al\nnúmero de entradas.",
        .position_zero = "La nueva posición debe ser\npositiva.",
        .jump_q = "¿A dónde quieres saltar?\nPuede hacer que los iconos se recarguen.",
        .cancel = "Cancelar",
        .jump = "Saltar",
        .no_theme_extdata = "¡Los datos de tema extensos no existen!\nEstablece un tema predeterminado desde el menú principal.",
        .loading_qr = "Cargando Escáner QR...",
        .no_wifi = "Por favor, conéctate a Wi-Fi antes de escanear códigos QR",
        .qr_homebrew = "El escaneo de QR no funciona desde el\nInicio, usa el navegador de ThemePlaza en su lugar.",
        .camera_broke = "Tu cámara parece tener un problema,\nno se pueden escanear códigos QR.",
        .too_many_themes = "Tienes demasiados temas seleccionados.",
        .not_enough_themes = "No tienes suficientes temas seleccionados.",
        .uninstall_confirm = "¿Estás seguro de que deseas eliminar\nel fondo instalado?",
        .delete_confirm = "¿Estás seguro de que deseas eliminar esto?",
    },
    .remote =
    {
        .no_results = "No hay resultados para esta búsqueda.",
        .check_wifi = "No se pudo descargar datos de Theme Plaza.\nAsegúrate de que el Wi-Fi esté activado.",
        .new_page_big = "La nueva página debe ser\nmenor o igual al\nnúmero de páginas.",
        .new_page_zero = "La nueva posición debe ser\npositiva.",
        .jump_page = "¿A qué página quieres saltar?",
        .cancel = "Cancelar",
        .jump = "Saltar",
        .tags = "¿Qué etiquetas deseas buscar?",
        .search = "Buscar",
        .parental_fail = "¡La validación del Control Parental falló!\nAcceso al navegador restringido.",
        .zip_not_found = "ZIP no encontrado en esta URL\nSi crees que esto es un error, por favor\ncontacta al administrador del sitio",
        .generic_httpc_error = "Error en el módulo sysmodule HTTPC - 0x%08lx.\nSi estás viendo esto, por favor contacta a un\ndesarrollador de Anemone en el Discord de Theme Plaza.",
        .http303_tp = "HTTP 303 Ver Otro (Theme Plaza)\n¿Se ha aprobado este tema?",
        .http303 = "HTTP 303 Ver Otro\nDescarga el recurso directamente\no contacta al administrador del sitio.",
        .http404 = "HTTP 404 No Encontrado\n¿Se ha aprobado este tema?",
        .http_err_url = "HTTP %s\nComprueba que la URL sea correcta.",
        .http_errcode_generic = "HTTP %s\nContacta al administrador del sitio.",
        .http401 = "401 No Autorizado",
        .http403 = "403 Prohibido",
        .http407 = "407 Autenticación de Proxy Requerida",
        .http414 = "HTTP 414 URI Demasiado Largo\nEl código QR apunta a una URL muy larga.\nDescarga el archivo directamente.",
        .http418 = "HTTP 418 Soy una tetera\nContacta al administrador del sitio.",
        .http426 = "HTTP 426 Se Requiere Actualización\nLa 3DS no puede conectarse a este servidor.\nContacta al administrador del sitio.",
        .http451 = "HTTP 451 No Disponible por Razones Legales\nAlguna entidad está impidiendo el acceso\nal servidor de alojamiento por razones legales.",
        .http500 = "HTTP 500 Error Interno del Servidor\nContacta al administrador del sitio.",
        .http502 = "HTTP 502 Puerta de Enlace Incorrecta\nContacta al administrador del sitio.",
        .http503 = "HTTP 503 Servicio No Disponible\nContacta al administrador del sitio.",
        .http504 = "HTTP 504 Tiempo de Espera de la Puerta de Enlace\nContacta al administrador del sitio.",
        .http_unexpected = "HTTP %u\nSi crees que esto es inesperado, por favor\ncontacta al administrador del sitio.",
    },
    .remote_instructions =
    {
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Descargar tema",
                    "\uE001 Volver"
                },
                {
                    "\uE002 Más opciones",
                    "\uE003 Previsualizar tema"
                },
                {
                    "\uE004 Página anterior",
                    "\uE005 Página siguiente"
                },
                {
                    "Salir",
                    NULL
                }
            }
        },
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Descargar fondo",
                    "\uE001 Volver"
                },
                {
                    "\uE002 Más opciones",
                    "\uE003 Previsualizar fondo"
                },
                {
                    "\uE004 Página anterior",
                    "\uE005 Página siguiente"
                },
                {
                    "Salir",
                    NULL
                }
            }
        },
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Descargar insignias",
                    "\uE001 Volver"
                },
                {
                    "\uE002 Más opciones",
                    "\uE003 Previsualizar insignias"
                },
                {
                    "\uE004 Pagina anterior",
                    "\uE005 Pagina siguiente"
                },
                {
                    "Exit",
                    NULL
                }
            }
        }
    },
    .remote_extra_instructions =
    {
        {
            .info_line = "\uE001 Dejar menú extra",
            .instructions = {
                {
                    "\uE079 Saltar a la página",
                    "\uE07A Buscar etiquetas"
                },
                {
                    NULL,
                    "\uE07C Recargar sin caché"
                },
                {
                    "\uE004 Cambiar Insignias",
                    "\uE005 Cambiar Splashes"
                },
                {
                    "Salir",
                    NULL
                }
            }
        },
        {
            .info_line = "\uE001 Dejar menú extra",
            .instructions = {
                {
                    "\uE079 Saltar a la página",
                    "\uE07A Buscar etiquetas"
                },
                {
                    NULL,
                    "\uE07C Recargar sin caché"
                },
                {
                    "\uE004 Cambiar Temas",
                    "\uE005 Cambiar Insignias"
                },
                {
                    "Salir",
                    NULL
                }
            }
        },
        {
            .info_line = "\uE001 Dejar menú extra",
            .instructions = {
                {
                    "\uE079 Saltar a la página",
                    "\uE07A Buscar etiquetas"
                },
                {
                    NULL,
                    "\uE07C Recargar sin caché"
                },
                {
                    "\uE004 Cambiar Splashes",
                    "\uE005 Cambiar Temas"
                },
                {
                    "Salir",
                    NULL
                }
            }
        },
    },
    .splashes =
    {
        .no_splash_found = "No se encontró splash.bin o splashbottom.bin.\n¿Es esto un fondo?",
        .splash_disabled = "ADVERTENCIA: Los fondos están desactivados en la Configuración de Luma",
    },
    .themes =
    {
        .no_body_found = "No se encontró body_LZ.bin - ¿Es esto un tema?",
        .mono_warn = "Uno o más temas instalados usan audio mono.\nEl audio mono causa varios problemas.\nConsulta la wiki para más información.",
        .illegal_char = "Se utilizó un carácter ilegal.",
        .name_folder = "Nombre de la carpeta de salida",
        .cancel = "Cancelar",
        .done = "Listo"
    },
    .badges =
    {
        .extdata_locked = "Datos Adicionales Bloqueados\nIntenta presionando el botón Home y vuelve a\nAnemone3DS, o usa la version CIA en su lugar.\nDebug: 0x%08lx"
    }
};


const Language_s language_french = {
    .normal_instructions = 
    {
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Installer",
                    "\uE001 Ajout thème aléatoire"
                },
                {
                    "\uE002 Plus d'options",
                    "\uE003 Aperçu"
                },
                {
                    "\uE004 Menu des splashs",
                    "\uE005 Scanner un QR code"
                },
                {
                    "Quitter",
                    "Supprimer"
                }
            }
        },
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Installer",
                    "\uE001 Effacer le splash installé"
                },
                {
                    "\uE002 Plus d'options",
                    "\uE003 Aperçu"
                },
                {
                    "\uE004 Menu des thèmes",
                    "\uE005 Scanner un QR code"
                },
                {
                    "Quitter",
                    "Supprimer"
                }
            }
        }
    },

    .install_instructions =
    {
        .info_line = "\uE001 Annuler l'installation",
        .instructions = {
            {
                "\uE079 Thème sélectionné",
                "\uE07A Thèmes aléatoire"
            },
            {
                "\uE07B Musique du thème",
                "\uE07C Thème sans musique"
            },
            {
                NULL,
                NULL
            },
            {
                "Quitter",
                NULL
            }
        }
    },

    .extra_instructions = 
    {
        {
            .info_line = "Trier... | \uE001 Retour",
            .instructions = {
                {
                    "\uE079 par nom",
                    "\uE07A par auteur"
                },
                {
                    "\uE07B par nom de fichier",
                    NULL
                },
                {
                    NULL,
                    NULL
                },
                {
                    "Quitter",
                    NULL
                }
            }
        },
        {
            .info_line = "\uE001 Retour",
            .instructions = {
                {
                    "\uE079 Aller à",
                    "\uE07A Actualiser les icônes"
                },
                {
                    "\uE07B Aller sur ThemePlaza",
                    "\uE07C Installer des Badges"
                },
                {
                    "\uE004 Trier...",
                    "\uE005 Dump..."
                },
                {
                    "Quitter",
                    NULL
                }
            }
        },
        {
            .info_line = "Dump... | \uE001 Retour",
            .instructions = {
                {
                    "\uE079 thème installé",
                    "\uE07A thèmes officiels"
                },
                {
                    "\uE07B tous les badges",
                    NULL
                },
                {
                    NULL,
                    NULL
                },
                {
                    "Quitter",
                    NULL
                }
            }
        }
    },
    .camera = 
    {
        .quit = "Appuyez sur \uE005 Pour quitter",
        .thread_error = "Capture cam thread creation failed\nVeuillez signaler ceci aux développeurs",
        .zip_not_theme_splash = "Le fichier zip téléchargé n'est\nni un thème, ni un splash",
        .file_not_zip = "Le fichier téléchargé n'est pas un zip.",
        .download_failed = "Le téléchargement a échoué.",
        .badge_question = "Un thème ou un splash n'a pas été detecté.\nEsc-ce un badge?",
    },
    .draw = 
    {
        .theme_mode = "Thèmes",
        .splash_mode = "Splashs",
        .no_themes = "Aucun thème trouvé",
        .no_splashes = "Aucun splash trouvé",
        .qr_download = "Appuyez sur \uE005 pour téléch. depuis un QR",
        .switch_splashes = "Ou \uE004 pour aller sur les splashs",
        .switch_themes = "Ou \uE004 pour aller sur les thèmes",
        .quit = "Ou        pour quitter",
        .start_pos = 142, // Adjust x pos of start glyph to line up with quit string
        .by = "Par ",
        .selected = "Sélectionné:",
        .sel = "Sél.:",
        .tp_theme_mode = "Thèmes sur ThemePlaza",
        .tp_splash_mode = "Splashs sur ThemePlaza",
        .tp_badge_mode = "Badges sur ThemePlaza",
        .search = "Rechercher...",
        .page = "Page:",
        .err_quit = "Appuyez sur \uE000 pour quitter.",
        .warn_continue = "Appuyez sur \uE000 pour continuer.",
        .yes_no = "\uE000 Oui   \uE001 Non",
        .load_themes = "Chargement des thèmes,\nveuillez patienter...",
        .load_splash = "Chargement des splashs,\nveuillez patienter...",
        .load_icons = "Chargement des icônes,\nveuillez patienter...",
        .install_splash = "Installation su splash...",
        .delete_splash = "Effacement du splash installé...",
        .install_theme = "Installation du thème...",
        .install_shuffle = "Installation de thèmes aléatoire...",
        .install_bgm = "Installation de la musique du thème...",
        .install_no_bgm = "Installation du thème sans la musique...",
        .downloading = "Téléchargement...",
        .checking_dl = "Vérification du fichier...",
        .delete_sd = "Effacement...",
        .download_themes = "Téléchargement de la liste des thèmes,\nveuillez patienter...",
        .download_splashes = "Téléchargement de la liste des splashs,\nveuillez patienter...",
        .download_badges = "Téléchargement de la liste des badges,\nveuillez patienter...",
        .download_preview = "Téléchargement de l'aperçu,\nveuillez patienter...",
        .download_bgm = "Téléchargement de la musique,\nveuillez patienter...",
        .dump_single = "Extraction du thème installé,\nveuillez patienter...",
        .dump_all_official = "Extraction des thèmes officiels,\nveuillez patienter...",
        .dump_badges = "Extraction des badges installés\nveuillez patienter...",
        .install_badges = "Installation des badges,\nveuillez patienter...",
        .shuffle = "Aléatoire: %i/10",
    },
    .fs =
    {
        .illegal_input = "Le texte ne doit pas contenir:\n" ILLEGAL_CHARS,
        .new_or_overwrite = "Choisissez un nouveau nom ou sélectionnez Écraser",
        .cancel = "Annuler",
        .overwrite = "Écraser",
        .rename = "Renommer",
        .swkbd_fail = "???\nTry a USB keyboard", // Should never be used
        .sd_full = "La carte SD est pleine.\nEffaçez du contenu pour faire de la place",
        .fs_error = "Erreur:\nPrenez une nouvelle carte SD.",
    },
    .loading =
    {
        .no_preview = "Aucun aperçu trouvé.",
    },
    .main =
    {
        .position_too_big = "La nouvelle position doit\nêtre + petite ou égale au\nnombre d'entrées!",
        .position_zero = "La nouvelle position\ndoit être positive!",
        .jump_q = "Où voulez vous aller?\nCela actualisera les icônes.",
        .cancel = "Annuler",
        .jump = "OK",
        .no_theme_extdata = "Les données additionnelles des thèmes\nn'existe pas! Changez de thème\ndepuis le menu home puis réessayez.",
        .loading_qr = "Chargement du scanneur de QR code...",
        .no_wifi = "Connectez vous à Internet\navant de scanner des QR codes.",
        .qr_homebrew = "La fonctionnalité de scanner des QR codes ne\nfonctionne pas depuis l'Homebrew Launcher,\nutilisez ThemePlaza à la place.",
        .camera_broke = "La caméra semble avoir un problème,\nimpossible de scanner de QR codes.",
        .too_many_themes = "Il y a trop de thèmes sélectionnés.",
        .not_enough_themes = "Il n'y a pas assez de thèmes sélectionnés.",
        .uninstall_confirm = "Voulez-vous supprimer le splash\nactuellement installé?",
        .delete_confirm = "Voulez-vous supprimer ceci?",
    },
    .remote =
    {
        .no_results = "Aucun résultat trouvé.",
        .check_wifi = "Impossible de télécharger les données\nde ThemePlaza. Vérifiez que vous soyez\nconnecté à Internet.",
        .new_page_big = "La nouvelle page doit\nêtre + petite ou égale au\nnombre de pages!",
        .new_page_zero = "La nouvelle position\ndoit être positive!",
        .jump_page = "Entrez la page sur laquelle aller",
        .cancel = "Annuler",
        .jump = "OK",
        .tags = "Quels tags voulez-vous rechercher?",
        .search = "Rechercher",
        .parental_fail = "Échec de la verification du contrôle parental.\nL'accès au site est restreint.",
        .zip_not_found = "Le ZIP n'a pas été trouvé sur cette URL.\nSi vous pensez que c'est une erreur,\ncontactez l'administrateur du site.",
        .generic_httpc_error = "Error in HTTPC sysmodule - 0x%08lx.\nSi vous voyez ceci, merci de contacter\nun développeur d'Anemone sur le serveur\nDiscord de ThemePlaza.",
        .http303_tp = "HTTP 303 See Other (Theme Plaza)\nLe thème est-il approuvé?",
        .http303 = "HTTP 303 See Other\nTéléchargez la ressource directement\nou contactez l'administrateur du site.",
        .http404 = "HTTP 404 Not Found\nLe thème est-il approuvé?",
        .http_err_url = "HTTP %s\nVérifiez que l'URL est correcte.",
        .http_errcode_generic = "HTTP %s\nContactez l'administrateur du site.",
        .http401 = "401 Unauthorized'",
        .http403 = "403 Forbidden",
        .http407 = "407 Proxy Authentication Required",
        .http414 = "HTTP 414 URI Too Long\nLe QR code pointe vers une URL trop longue.\nTéléchargez le fichier directement.",
        .http418 = "HTTP 418 I'm a teapot\nContactez l'administrateur du site.",
        .http426 = "HTTP 426 Upgrade Required\nLa 3DS ne peut pas se connecter.\nContactez l'administrateur du site.",
        .http451 = "HTTP 451 Unavailable for Legal Reasons\nL'accès au serveur est restreint\npour des raisons légales.",
        .http500 = "HTTP 500 Internal Server Error\nContactez l'administrateur du site.",
        .http502 = "HTTP 502 Bad Gateway\nContactez l'administrateur du site.",
        .http503 = "HTTP 503 Service Unavailable\nContactez l'administrateur du site.",
        .http504 = "HTTP 504 Gateway Timeout\nContactez l'administrateur du site.",
        .http_unexpected = "HTTP %u\nSi vous pensez que ceci est inattendu,\ncontactez l'administrateur du site.",
    },
    .remote_instructions =
    {
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Télécharger",
                    "\uE001 Retour"
                },
                {
                    "\uE002 Plus d'options",
                    "\uE003 Aperçu"
                },
                {
                    "\uE004 Page précédente",
                    "\uE005 Page suivante"
                },
                {
                    "Quitter",
                    NULL
                }
            }
        },
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Télécharger",
                    "\uE001 Retour"
                },
                {
                    "\uE002 Plus d'options",
                    "\uE003 Aperçu"
                },
                {
                    "\uE004 Page précédente",
                    "\uE005 Page suivante"
                },
                {
                    "Quitter",
                    NULL
                }
            }
        },
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Télécharger",
                    "\uE001 Retour"
                },
                {
                    "\uE002 Plus d'options",
                    "\uE003 Aperçu"
                },
                {
                    "\uE004 Page précédente",
                    "\uE005 Page suivante"
                },
                {
                    "Quitter",
                    NULL
                }
            }
        }
    },
    .remote_extra_instructions =
    {
        {
            .info_line = "\uE001 Retour",
            .instructions = {
                {
                    "\uE079 Aller à la page",
                    "\uE07A Recherche par tag"
                },
                {
                    NULL,
                    "\uE07C Actualiser sans cache"
                },
                {
                    "\uE004 Naviguer les Badges",
                    "\uE005 Naviguer les Splashs"
                },
                {
                    "Quitter",
                    NULL
                }
            },
        },
                {
            .info_line = "\uE001 Retour",
            .instructions = {
                {
                    "\uE079 Aller à la page",
                    "\uE07A Tags de recherche"
                },
                {
                    NULL,
                    "\uE07C Actualiser sans cache"
                },
                {
                    "\uE004 Naviguer les Thèmes",
                    "\uE005 Naviguer les Badges"
                },
                {
                    "Quitter",
                    NULL
                }
            },
        },
                {
            .info_line = "\uE001 Retour",
            .instructions = {
                {
                    "\uE079 Aller à la page",
                    "\uE07A Tags de recherche"
                },
                {
                    NULL,
                    "\uE07C Actualiser sans cache"
                },
                {
                    "\uE004 Naviguer les Splashs",
                    "\uE005 Naviguer les Thèmes"
                },
                {
                    "Quitter",
                    NULL
                }
            },
        }
    },
    .splashes =
    {
        .no_splash_found = "Aucun splash.bin ou splashbottom.bin trouvé.\nEst-ce un splash?",
        .splash_disabled = "ATTENTION: Les Splashs sont désactivés\ndans la configuration de Luma",
    },
    .themes =
    {
        .no_body_found = "Aucun body_LZ.bin trouvé.\nEst-ce un theme?",
        .mono_warn = "Un ou plusieurs thèmes installé\nutilise de l'audio mono.\nCeci peut causer des problèmes.\nRegardez le wiki pour plus d'information.",
        .illegal_char = "Caractère interdit utilisé.",
        .name_folder = "Nom du dossier de destination",
        .cancel = "Annuler",
        .done = "OK"
    },
    .badges = 
    {
        .extdata_locked = "L'archive des badges est vérouillée.\nEssayez de redémarrer Anemone3DS,\nou utilisez la version CIA.\nDebug: 0x%08lx"
    }
};

const Language_s language_portuguese = {
    .normal_instructions = 
    {
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Instalar Tema(s)",
                    "\uE001 Adicionar ao shuffle"
                },
                {
                    "\uE002 Mais opções",
                    "\uE003 Pré-visualizar"
                },
                {
                    "\uE004 Mudar para Splashes",
                    "\uE005 Escanear código QR"
                },
                {
                    "Sair",
                    "Excluir do SD"
                }
            }
        },
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Instalar Splash",
                    "\uE001 Excluir splash instalado"
                },
                {
                    "\uE002 Mais opções",
                    "\uE003 Pré-visualizar"
                },
                {
                    "\uE004 Mudar para Temas",
                    "\uE005 Escanear código QR"
                },
                {
                    "Sair",
                    "Excluir do SD"
                }
            }
        }
    },

    .install_instructions =
    {
        .info_line = "\uE001 Cancelar instalação do tema",
        .instructions = {
            {
                "\uE079 Instalação normal",
                "\uE07A Instalação shuffle"
            },
            {
                "\uE07B Instalar só BGM",
                "\uE07C Instalar sem BGM"
            },
            {
                NULL,
                NULL
            },
            {
                "Sair",
                NULL
            }
        }
    },

    .extra_instructions = 
    {
        {
            .info_line = "\uE001 Voltar",
            .instructions = {
                {
                    "\uE079 Classificar por nome",
                    "\uE07A Classificar por autor"
                },
                {
                    "\uE07B Classificar por arquivo",
                    NULL
                },
                {
                    NULL,
                    NULL
                },
                {
                    "Sair",
                    NULL
                }
            }
        },
        {
            .info_line = "\uE001 Voltar",
            .instructions = {
                {
                    "\uE079 Ir à página",
                    "\uE07A Att. icones quebrados"
                },
                {
                    "\uE07B Modo ThemePlaza",
                    "\uE07C Instalar Insígnias"
                },
                {
                    "\uE004 Classificar por:",
                    "\uE005 Menu de exportação"
                },
                {
                    "Sair",
                    NULL
                }
            }
        },
        {
            .info_line = "\uE001 Voltar",
            .instructions = {
                {
                    "\uE079 Exportar Tema Atual",
                    "\uE07A Exportar Todos Temas"
                },
                {
                    "\uE07B Exportar Insígnias",
                    NULL
                },
                {
                    NULL,
                    NULL
                },
                {
                    "Sair",
                    NULL
                }
            }
        }
    },
    .camera = 
    {
        .quit = "Aperte \uE005 para Sair",
        .thread_error = "Capture cam thread creation failed\nPor favor, Informe isso aos desenvolvedores",
        .zip_not_theme_splash = "O zip baixado não é um splash nem um tema",
        .file_not_zip = "O arquivo baixado não é um zip.",
        .download_failed = "Falha no download.",
        .badge_question = "Tema ou splash não detectado.\nIsso é uma insígnia?",
    },
    .draw = 
    {
        .theme_mode = "Modo Tema",
        .splash_mode = "Modo Splash",
        .no_themes = "Nenhum tema encontrado",
        .no_splashes = "Nenhum splash encontrado",
        .qr_download = "Aperte \uE005 para baixar do QR",
        .switch_splashes = "Ou \uE004 para mudar para splashes",
        .switch_themes = "Ou \uE004 para mudar para temas",
        .quit = "Ou        para sair",
        .start_pos = 165, // Adjust x pos of start glyph to line up with quit string
        .by = "Por ",
        .selected = "Selecionado:",
        .sel = "Sel.:",
        .tp_theme_mode = "Modo de Tema ThemePlaza",
        .tp_splash_mode = "Modo de Splash ThemePlaza",
        .tp_badge_mode = "Modo de Insígnia ThemePlaza",
        .search = "Pesquisar...",
        .page = "Página:",
        .err_quit = "Aperte \uE000 para sair.",
        .warn_continue = "Aperte \uE000 para continuar.",
        .yes_no = "\uE000 Sim   \uE001 Não",
        .load_themes = "Carregando temas, aguarde...",
        .load_splash = "Carregando splashes, aguarde...",
        .load_icons = "Carregando ícones, aguarde...",
        .install_splash = "Instalando um splash...",
        .delete_splash = "Excluindo splash instalado...",
        .install_theme = "Instalando um único tema...",
        .install_shuffle = "Instalando temas shuffle...",
        .install_bgm = "Instalando somente o BGM...",
        .install_no_bgm = "Instalando tema sem BGM...",
        .downloading = "Baixando...",
        .checking_dl = "Verificando arquivo instalado...",
        .delete_sd = "Deletando do SD...",
        .download_themes = "Baixando a lista de temas, aguarde...",
        .download_splashes = "Baixando a lista de splashes, aguarde...",
        .download_badges = "Baixando a lista de insígnias, aguarde...",
        .download_preview = "Baixando prévia, aguarde...",
        .download_bgm = "Baixando BGM, aguarde...",
        .dump_single = "Exportando tema, aguarde...",
        .dump_all_official = "Exportando temas oficiais, aguarde...",
        .dump_badges = "Exportando insígnias instaladas, aguarde...",
        .install_badges = "Instalando insígnias, aguarde...",
        .shuffle = "Shuffle: %i/10",
    },
    .fs =
    {
        .illegal_input = "A entrada não deve conter:\n" ILLEGAL_CHARS,
        .new_or_overwrite = "Escolha um novo nome ao arquivo ou Sobrescreva",
        .cancel = "Cancelar",
        .overwrite = "Sobrescrever",
        .rename = "Renomear",
        .swkbd_fail = "???\nTry a USB keyboard", // Should never be used
        .sd_full = "O cartão SD está cheio.\nExclua alguns temas para liberar espaço.",
        .fs_error = "Erro:\nAdquira um novo cartão SD.",
    },
    .loading =
    {
        .no_preview = "Prévia não encontrada.",
    },
    .main =
    {
        .position_too_big = "A nova posição deve ser\nmenor ou igual ao\nnúmero de entradas!",
        .position_zero = "A nova posição precisa\nser positiva!",
        .jump_q = "Para qual página deseja ir?\nPode causar recarregamento de ícones.",
        .cancel = "Cancelar",
        .jump = "Ir",
        .no_theme_extdata = "O extdata do tema não existe!\nDefina um tema padrão no menu HOME.",
        .loading_qr = "Carregando o leitor QR...",
        .no_wifi = "Conecte-se ao Wi-Fi antes de escanear códigos QR",
        .qr_homebrew = "O leitor QR não funciona no Homebrew\nLauncher, use o navegador ThemePlaza em vez disso.",
        .camera_broke = "Sua câmera parece ter um problema,\nincapaz de escanear códigos QR.",
        .too_many_themes = "Você tem muitos temas selecionados.",
        .not_enough_themes = "Você não tem temas suficientes selecionados.",
        .uninstall_confirm = "Tem certeza de que deseja excluir\no splash instalado?",
        .delete_confirm = "Tem certeza de que deseja excluir isso?",
    },
    .remote =
    {
        .no_results = "Sem resultados para esta pesquisa.",
        .check_wifi = "Não foi possível baixar os dados do Theme Plaza.\nCertifique-se de que o Wi-Fi esteja ligado.",
        .new_page_big = "A nova página deve ser\nmenor ou igual ao\nnúmero de páginas!",
        .new_page_zero = "A nova posição precisa\nser positiva!",
        .jump_page = "Para qual página deseja ir?",
        .cancel = "Cancelar",
        .jump = "Ir",
        .tags = "Quais tags deseja pesquisar?",
        .search = "Pesquisar",
        .parental_fail = "A validação dos controles parentais falhou!\nNavegador restrito.",
        .zip_not_found = "ZIP não encontrado neste URL\nSe achar que isso é um erro,\ncontate o administrador do site",
        .generic_httpc_error = "Error in HTTPC sysmodule - 0x%08lx.\nSe estiver vendo isso, contate um\ndesenvolvedor do Anenome no discord Theme Plaza.",
        .http303_tp = "HTTP 303 See Other (Theme Plaza)\nEsse tema foi aprovado?",
        .http303 = "HTTP 303 See Other\nBaixe o recurso diretamente\nou contate o administrador do site.",
        .http404 = "HTTP 404 Not Found\nEsse tema foi aprovado?",
        .http_err_url = "HTTP %s\nCertifique-se de que o URL esteja correto.",
        .http_errcode_generic = "HTTP %s\nContate o administrador do site.",
        .http401 = "401 Unauthorized",
        .http403 = "403 Forbidden",
        .http407 = "407 Proxy Authentication Required",
        .http414 = "HTTP 414 URI Too Long\nO código QR aponta para um URL muito longo.\nBaixe o arquivo diretamente.",
        .http418 = "HTTP 418 I'm a teapot\nContate o administrador do site.",
        .http426 = "HTTP 426 Upgrade Required\nO 3DS não consegue acessar este servidor.\nContate o administrador do site.",
        .http451 = "HTTP 451 Unavailable for Legal Reasons\nAlguma entidade está impedindo\no acesso ao servidor host por razões legais.",
        .http500 = "HTTP 500 Internal Server Error\nContate o administrador do site.",
        .http502 = "HTTP 502 Bad Gateway\nContate o administrador do site.",
        .http503 = "HTTP 503 Service Unavailable\nContate o administrador do site.",
        .http504 = "HTTP 504 Gateway Timeout\nContate o administrador do site.",
        .http_unexpected = "HTTP %u\nSe achar que isso é inesperado,\ncontate o administrador do site.",
    },
    .remote_instructions =
    {
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Baixar Tema",
                    "\uE001 Voltar"
                },
                {
                    "\uE002 Mais opções",
                    "\uE003 Pré-visualizar"
                },
                {
                    "\uE004 Página anterior",
                    "\uE005 Próxima página"
                },
                {
                    "Sair",
                    NULL
                }
            }
        },
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Baixar Splash",
                    "\uE001 Voltar"
                },
                {
                    "\uE002 Mais opções",
                    "\uE003 Pré-visualizar"
                },
                {
                    "\uE004 Página anterior",
                    "\uE005 Próxima página"
                },
                {
                    "Sair",
                    NULL
                }
            }
        },
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Baixar Insígnias",
                    "\uE001 Voltar"
                },
                {
                    "\uE002 Mais opções",
                    "\uE003 Pré-visualizar"
                },
                {
                    "\uE004 Página anterior",
                    "\uE005 Próxima página"
                },
                {
                    "Sair",
                    NULL
                }
            }
        }
    },
    .remote_extra_instructions =
    {
        {
            .info_line = "\uE001 Voltar",
            .instructions = {
                {
                    "\uE079 Ir à página",
                    "\uE07A Pesquisar tags"
                },
                {
                    NULL,
                    "\uE07C Recarregar sem cache"
                },
                {
                    "\uE004 Mudar p. Insígnias",
                    "\uE005 Mudar p. Splashes"
                },
                {
                    "Sair",
                    NULL
                }
            }
        },
        {
            .info_line = "\uE001 Voltar",
            .instructions = {
                {
                    "\uE079 Ir à página",
                    "\uE07A Pesquisar tags"
                },
                {
                    NULL,
                    "\uE07C Recarregar sem cache"
                },
                {
                    "\uE004 Mudar p. Temas",
                    "\uE005 Mudar p. Insígnias"
                },
                {
                    "Sair",
                    NULL
                }
            }
        },
        {
            .info_line = "\uE001 Voltar",
            .instructions = {
                {
                    "\uE079 Ir à página",
                    "\uE07A Pesquisar tags"
                },
                {
                    NULL,
                    "\uE07C Recarregar sem cache"
                },
                {
                    "\uE004 Mudar p. Splashes",
                    "\uE005 Mudar p. Temas"
                },
                {
                    "Sair",
                    NULL
                }
            }
        },
    },
    .splashes =
    {
        .no_splash_found = "Não foi encontrado splash.bin ou splashbottom.bin.\nIsso é um splash?",
        .splash_disabled = "AVISO: Splashes estão desativados na config. Luma.",
    },
    .themes =
    {
        .no_body_found = "Não foi encontrado body_LZ.bin - isso é um tema?",
        .mono_warn = "Um ou mais temas instalados usam áudio mono. O áudio mono causa vários problemas. Consulte a wiki para mais informações.",
        .illegal_char = "Caractere ilegal usado.",
        .name_folder = "Nome da pasta de saída",
        .cancel = "Cancelar",
        .done = "Pronto"
    },
    .badges = 
    {
        .extdata_locked = "Ext Data Bloqueado\nTente apertar o botão HOME e retornar\nao Anemone3DS, ou use a versão CIA.\nDebug: 0x%08lx"
    }
};

const Language_s language_korean = {
    // As you can see, this isn't korean. This is a placeholder so that the text is visible correctly because the Korean font is big.
    // Translations are welcomed!
    .normal_instructions = 
    {
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Install Theme(s)",
                    "\uE001 Queue shuffle"
                },
                {
                    "\uE002 More options",
                    "\uE003 Preview theme"
                },
                {
                    "\uE004 Go to splashes",
                    "\uE005 Scan QR code"
                },
                {
                    "Exit",
                    "Delete from SD"
                }
            }
        },
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Install splash",
                    "\uE001 Delete cur. splash"
                },
                {
                    "\uE002 More options",
                    "\uE003 Preview splash"
                },
                {
                    "\uE004 Go to themes",
                    "\uE005 Scan QR code"
                },
                {
                    "Exit",
                    "Delete from SD"
                }
            }
        }
    },

    .install_instructions =
    {
        .info_line = "\uE001 Cancel theme install",
        .instructions = {
            {
                "\uE079 Normal install",
                "\uE07A Shuffle install"
            },
            {
                "\uE07B BGM-only install",
                "\uE07C No-BGM install"
            },
            {
                NULL,
                NULL
            },
            {
                "Exit",
                NULL
            }
        }
    },

    .extra_instructions = 
    {
        {
            .info_line = "\uE001 Leave sorting menu",
            .instructions = {
                {
                    "\uE079 Sort by name",
                    "\uE07A Sort by author"
                },
                {
                    "\uE07B Sort by filename",
                    NULL
                },
                {
                    NULL,
                    NULL
                },
                {
                    "Exit",
                    NULL
                }
            }
        },
        {
            .info_line = "\uE001 Leave extra menu",
            .instructions = {
                {
                    "\uE079 Jump in the list",
                    "\uE07A Reload icons"
                },
                {
                    "\uE07B Browse TP",
                    "\uE07C Install Badges"
                },
                {
                    "\uE004 Sorting menu",
                    "\uE005 Dump..."
                },
                {
                    "Exit",
                    NULL
                }
            }
        },
        {
            .info_line = "Dump... | \uE001 Leave dump menu",
            .instructions = {
                {
                    "\uE079 Installed Theme",
                    "\uE07A All Themes"
                },
                {
                    "\uE07B Badges",
                    NULL
                },
                {
                    NULL,
                    NULL
                },
                {
                    "Exit",
                    NULL
                }
            }
        }
    },
    .camera = 
    {
        .quit = "Press \uE005 To Quit",
        .thread_error = "Capture cam thread creation failed\nPlease report this to the developers",
        .zip_not_theme_splash = "Zip downloaded is neither\na splash nor a theme",
        .file_not_zip = "File downloaded isn't a zip.",
        .download_failed = "Download failed.",
        .badge_question = "Theme or splash not detected.\nIs this a badge?",
    },
    .draw = 
    {
        .theme_mode = "Theme mode",
        .splash_mode = "Splash mode",
        .no_themes = "No theme found",
        .no_splashes = "No splash found",
        .qr_download = "Press \uE005 to dl from QR",
        .switch_splashes = "Or \uE004 to go to splashes",
        .switch_themes = "Or \uE004 to go to themes",
        .quit = "Or        to quit",
        .start_pos = 162, // Adjust x pos of start glyph to line up with quit string
        .by = "By ",
        .selected = "Selected:",
        .sel = "Sel.:",
        .tp_theme_mode = "ThemePlaza Theme mode",
        .tp_splash_mode = "ThemePlaza Splash mode",
        .tp_badge_mode = "ThemePlaza Badge mode",
        .search = "Search...",
        .page = "Page:",
        .err_quit = "Press \uE000 to quit.",
        .warn_continue = "Press \uE000 to continue.",
        .yes_no = "\uE000 Yes   \uE001 No",
        .load_themes = "Loading themes, please wait...",
        .load_splash = "Loading splashes, please wait...",
        .load_icons = "Loading icons, please wait...",
        .install_splash = "Installing a splash...",
        .delete_splash = "Deleting installed splash...",
        .install_theme = "Installing a single theme...",
        .install_shuffle = "Installing shuffle themes...",
        .install_bgm = "Installing BGM-only theme...",
        .install_no_bgm = "Installing theme w/o BGM...",
        .downloading = "Downloading...",
        .checking_dl = "Checking downloaded file...",
        .delete_sd = "Deleting from SD...",
        .download_themes = "Downloading theme list,\nplease wait...",
        .download_splashes = "Downloading splash list,\nplease wait...",
        .download_badges = "Downloading badge list,\nplease wait...",
        .download_preview = "Downloading preview,\nplease wait...",
        .download_bgm = "Downloading BGM, please wait...",
        .dump_single = "Dumping theme, please wait...",
        .dump_all_official = "Dumping official themes,\nplease wait...",
        .dump_badges = "Dumping installed badges,\nplease wait...",
        .install_badges = "Installing badges, please wait...",
        .shuffle = "Shuffle: %i/10",
    },
    .fs =
    {
        .illegal_input = "Input must not contain:\n" ILLEGAL_CHARS,
        .new_or_overwrite = "Choose a new filename or tap Overwrite",
        .cancel = "Cancel",
        .overwrite = "Overwrite",
        .rename = "Rename",
        .swkbd_fail = "???\nTry a USB keyboard", // Should never be used
        .sd_full = "SD card is full.\nDelete some themes to make space.",
        .fs_error = "Error:\nGet a new SD card.",
    },
    .loading =
    {
        .no_preview = "No preview found.",
    },
    .main =
    {
        .position_too_big = "The new position has to be\nsmaller or equal to the\nnumber of entries!",
        .position_zero = "The new position has to\nbe positive!",
        .jump_q = "Where do you want to jump to?\nMay cause icons to reload.",
        .cancel = "Cancel",
        .jump = "Jump",
        // Note to translator: This is a special case, please translate this string instead of the original one.
        .no_theme_extdata = "Theme extdata does not exist!\nPlease install the Korean patch\nthen set a default theme\nfrom the homemenu settings.",
        .loading_qr = "Loading QR Scanner...",
        .no_wifi = "Please connect to Wi-Fi\nbefore scanning QR codes",
        .qr_homebrew = "QR scanning doesnt work from the Homebrew\nLauncher, use the ThemePlaza browser instead.",
        .camera_broke = "Your camera seems to have a problem,\nunable to scan QR codes.",
        .too_many_themes = "You have too many themes selected.",
        .not_enough_themes = "You don't have enough themes selected.",
        .uninstall_confirm = "Are you sure you would like to delete\nthe installed splash?",
        .delete_confirm = "Are you sure you would like\nto delete this?",
    },
    .remote =
    {
        .no_results = "No results for this search.",
        .check_wifi = "Couldn't download Theme Plaza data.\nMake sure WiFi is on.",
        .new_page_big = "The new page has to be\nsmaller or equal to the\nnumber of pages!",
        .new_page_zero = "The new position has to\nbe positive!",
        .jump_page = "Which page do you want to jump to?",
        .cancel = "Cancel",
        .jump = "Jump",
        .tags = "Which tags do you want to search for?",
        .search = "Search",
        .parental_fail = "Parental Control validation failed!\nBrowser Access restricted.",
        .zip_not_found = "ZIP not found at this URL\nIf you believe this is an error, please\ncontact the site administrator",
        .generic_httpc_error = "Error in HTTPC sysmodule - 0x%08lx.\nIf you are seeing this, please contact an\nAnemone developer on the Theme Plaza Discord.",
        .http303_tp = "HTTP 303 See Other (Theme Plaza)\nHas this theme been approved?",
        .http303 = "HTTP 303 See Other\nDownload the resource directly\nor contact the site administrator.",
        .http404 = "HTTP 404 Not Found\nHas this theme been approved?",
        .http_err_url = "HTTP %s\nCheck that the URL is correct.",
        .http_errcode_generic = "HTTP %s\nContact the site administrator.",
        .http401 = "401 Unauthorized",
        .http403 = "403 Forbidden",
        .http407 = "407 Proxy Authentication Required",
        .http414 = "HTTP 414 URI Too Long\nThe QR code points to a really long URL.\nDownload the file directly.",
        .http418 = "HTTP 418 I'm a teapot\nContact the site administrator.",
        .http426 = "HTTP 426 Upgrade Required\nThe 3DS cannot connect to this server.\nContact the site administrator.",
        .http451 = "HTTP 451 Unavailable for Legal Reasons\nSome entity is preventing access\nto the host server for legal reasons.",
        .http500 = "HTTP 500 Internal Server Error\nContact the site administrator.",
        .http502 = "HTTP 502 Bad Gateway\nContact the site administrator.",
        .http503 = "HTTP 503 Service Unavailable\nContact the site administrator.",
        .http504 = "HTTP 504 Gateway Timeout\nContact the site administrator.",
        .http_unexpected = "HTTP %u\nIf you believe this is unexpected, please\ncontact the site administrator.",
    },
    .remote_instructions =
    {
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Download theme",
                    "\uE001 Go back"
                },
                {
                    "\uE002 More options",
                    "\uE003 Preview theme"
                },
                {
                    "\uE004 Previous page",
                    "\uE005 Next page"
                },
                {
                    "Exit",
                    NULL
                }
            }
        },
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Download splash",
                    "\uE001 Go back"
                },
                {
                    "\uE002 More options",
                    "\uE003 Preview splash"
                },
                {
                    "\uE004 Previous page",
                    "\uE005 Next page"
                },
                {
                    "Exit",
                    NULL
                }
            }
        },
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Download badges",
                    "\uE001 Go back"
                },
                {
                    "\uE002 More options",
                    "\uE003 Preview badges"
                },
                {
                    "\uE004 Previous page",
                    "\uE005 Next page"
                },
                {
                    "Exit",
                    NULL
                }
            }
        }
    },
    .remote_extra_instructions =
    {
        {
            .info_line = "\uE001 Leave extra menu",
            .instructions = {
                {
                    "\uE079 Jump to page",
                    "\uE07A Search tags"
                },
                {
                    NULL,
                    "\uE07C Reload w/o cache"
                },
                {
                    "\uE004 Go to Badges",
                    "\uE005 Go to Splashes"
                },
                {
                    "Exit",
                    NULL
                }
            }
        },
        {
            .info_line = "\uE001 Leave extra menu",
            .instructions = {
                {
                    "\uE079 Jump to page",
                    "\uE07A Search tags"
                },
                {
                    NULL,
                    "\uE07C Reload w/o cache"
                },
                {
                    "\uE004 Go to Themes",
                    "\uE005 Go to Badges"
                },
                {
                    "Exit",
                    NULL
                }
            }
        },
        {
            .info_line = "\uE001 Leave extra menu",
            .instructions = {
                {
                    "\uE079 Jump to page",
                    "\uE07A Search tags"
                },
                {
                    NULL,
                    "\uE07C Reload w/o cache"
                },
                {
                    "\uE004 Go to Splashes",
                    "\uE005 Go to Themes"
                },
                {
                    "Exit",
                    NULL
                }
            }
        }
    },
    .splashes =
    {
        .no_splash_found = "No splash.bin or splashbottom.bin found.\nIs this a splash?",
        .splash_disabled = "WARNING: Splashes are disabled in Luma Config",
    },
    .themes =
    {
        .no_body_found = "No body_LZ.bin found - is this a theme?",
        .mono_warn = "One or more installed themes\nuse mono audio.\nMono audio causes a number of issues.\nCheck the wiki for more information.",
        .illegal_char = "Illegal character used.",
        .name_folder = "Name of output folder",
        .cancel = "Cancel",
        .done = "Done"
    },
    .badges = 
    {
        .extdata_locked = "Ext Data Locked\nTry pressing the Home Button\nand then returning to Anemone3DS,\nor using the CIA version instead.\nDebug: 0x%08lx"
    }
};

Language_s init_strings(CFG_Language lang)
{
    switch (lang)
    {
        //case CFG_LANGUAGE_JP:
        case CFG_LANGUAGE_FR:
            return language_french;
        //case CFG_LANGUAGE_DE:
        //case CFG_LANGUAGE_IT:
        case CFG_LANGUAGE_ES:
            return language_spanish;
        //case CFG_LANGUAGE_ZH:
        case CFG_LANGUAGE_KO:
            return language_korean;
        //case CFG_LANGUAGE_NL:
        case CFG_LANGUAGE_PT:
            return language_portuguese;
        //case CFG_LANGUAGE_RU:
        //case CFG_LANGUAGE_TW:
        case CFG_LANGUAGE_EN:
            return language_english;
        default:
            return language_english;
    }
}

CFG_Language get_system_language(void)
{
    u8 lang = CFG_LANGUAGE_EN;
    // can never fail, cfguInit is one of the very first thing that happens on start
    // and if it does anyway, default to english
    CFGU_GetSystemLanguage(&lang);
    return (CFG_Language)lang;
}
