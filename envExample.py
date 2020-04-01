Import("env")

env.Replace(
    UPLOAD_PORT="1.1.1.1",  # IP of device for OTA update
    UPLOAD_FLAGS=["--auth=PASSWORD"]  # Arduino OTA authentication password
)
