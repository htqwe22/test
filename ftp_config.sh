CUR_USER=$USER
sudo apt-get install vsftpd
echo "backup"
sudo cp /etc/vsftpd.conf /etc/vsftpd.conf.bak_
echo "config"
sudo cp vsftpd.conf /etc/
cd ~
mkdir -p ~/ftp/pub
chmod 751 ftp
chmod 777 ~/ftp/pub 
sudo service vsftpd restart
#sudo usermod -a -G ftp $CUR_USER
#sudo usermod -a -G $CUR_USER ftp
