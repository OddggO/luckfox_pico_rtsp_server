编译流程: 
1. 在在目录'luckfox_pico_rkmpi_example'设置sdk目录, export LUCKFOX_SDK_PATH=/home/luckfox-pico. 其目的是找到tools目录下编译工具链地址
2. ./build.sh编译, 选择uclibc

上传到板端: 注意, typec接口要用电脑端口供端, 否则无法ssh进入
scp -r install/uclibc/luckfox_pico_rtsp_server_demo/ root@172.32.0.93:/root

目前已完成yolov5推理 + sort算法跟踪，接下来考虑应用deepsort算法或替换为yolov8
