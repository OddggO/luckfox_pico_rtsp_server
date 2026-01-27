rtsp服务器实现 + rknn yolov5推理 + sort跟踪算法

编译流程: 
1. 在在目录'luckfox_pico_rkmpi_example'设置sdk目录, export LUCKFOX_SDK_PATH=/home/luckfox-pico. 其目的是找到tools目录下编译工具链地址
2. ./build.sh编译, 选择uclibc

上传到板端: 注意, typde要用电脑端口供端, 否则无法ssh进入
scp -r install root@172.32.0.93:/root
