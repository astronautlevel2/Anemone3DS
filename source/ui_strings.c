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
                    NULL
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
                    NULL,
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
        .start_pos = 162, // Ajustar posición x del glifo de inicio para alinear con la cadena de salida
        .by = "Por ",
        .selected = "Seleccionado:",
        .sel = "Sel.:",
        .tp_theme_mode = "Modo tema de ThemePlaza",
        .tp_splash_mode = "Modo fondo de ThemePlaza",
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
        .download_themes = "Descargando lista de temas, por favor espera...",
        .download_splashes = "Descargando lista de fondos, por favor espera...",
        .download_preview = "Descargando vista previa, por favor espera...",
        .download_bgm = "Descargando BGM, por favor espera...",
        .dump_single = "Volcando tema, por favor espera...",
        .dump_all_official = "Volcando temas oficiales, por favor espera...",
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
                    "\uE003 Vista previa del tema"
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
                    "\uE003 Vista previa del fondo"
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
        }
    },
    .remote_extra_instructions =
    {
        .info_line = "\uE001 Dejar menú extra",
        .instructions = {
            {
                "\uE079 Saltar a la página",
                "\uE07A Buscar etiquetas"
            },
            {
                "\uE07B Alternar fondo/tema",
                "\uE07C Recargar sin caché"
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
    .splashes =
    {
        .no_splash_found = "No se encontró splash.bin o splashbottom.bin.\n¿Es esto un fondo?",
        .splash_disabled = "ADVERTENCIA: Los fondos están desactivados en la Configuración de Luma",
    },
    .themes =
    {
        .no_body_found = "No se encontró body_LZ.bin - ¿es esto un tema?",
        .mono_warn = "Uno o más temas instalados usan audio mono.\nEl audio mono causa varios problemas.\nConsulta la wiki para más información.",
        .illegal_char = "Se utilizó un carácter ilegal.",
        .name_folder = "Nombre de la carpeta de salida",
        .cancel = "Cancelar",
        .done = "Listo"
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
            .info_line = "\uE001 Retour",
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
                    NULL
                },
                {
                    "\uE004 Trier...",
                    "\uE005 Dump thèmes officiels"
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
                    "\uE079 Dump thème installé",
                    "\uE07A Dump tous les thèmes"
                },
                {
                    NULL,
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
        .tp_theme_mode = "Thèmes de ThemePlaza",
        .tp_splash_mode = "Splashs de ThemePlaza",
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
        .download_preview = "Téléchargement de l'aperçu,\nveuillez patienter...",
        .download_bgm = "Téléchargement de la musique,\nveuillez patienter...",
        .dump_single = "Extraction du thème installé,\nveuillez patienter...",
        .dump_all_official = "Extraction des thèmes officiels,\nveuillez patienter...",
        .shuffle = "Aléatoire: %i/10",
    },
    .fs =
    {
        .illegal_input = "Input must not contain:\n" ILLEGAL_CHARS,
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
        }
    },
    .remote_extra_instructions =
    {
        .info_line = "\uE001 Retour",
        .instructions = {
            {
                "\uE079 Aller à la page",
                "\uE07A Tags de recherche"
            },
            {
                "\uE07B Splash/Thème",
                "\uE07C Actualiser sans cache"
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
        //case CFG_LANGUAGE_KO:
        //case CFG_LANGUAGE_NL:
        //case CFG_LANGUAGE_PT:
        //case CFG_LANGUAGE_RU:
        //case CFG_LANGUAGE_TW:
        case CFG_LANGUAGE_EN:
            return language_spanish;
        default:
            return language_spanish;
    }
}
