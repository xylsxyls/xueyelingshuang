#sudo apt install ffmpeg -y
#sudo apt-get install ubuntu-restricted-extras
ffmpeg -f x11grab -s $(xdpyinfo | grep 'dimensions:'|awk '{print $2}') -i $DISPLAY -r 25 -c:v libx264 -pix_fmt yuv420p -preset ultrafast -qp 0 sample.mp4
