set(include_dirs "include")
set(srcs "app_driver.c" "main.c")
set(COMPONENT_EMBED_TXTFILES
    "certificate/prvtkey.pem" 
    "certificate/servercert.pem"
    "certificate/server.key"
    "certificate/server.crt"
    )

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dirs}" "."
                       REQUIRES app_storage light_driver button esp_wifi app_network esp_rainmaker esp_https_server esp_local_ctrl mdns bt
                       EMBED_TXTFILES "${COMPONENT_EMBED_TXTFILES}")