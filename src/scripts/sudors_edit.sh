#Execute create_ap without asking password
echo 'ALL ALL=NOPASSWD: /usr/bin/create_ap' | sudo EDITOR='tee -a' visudo -f /etc/sudoers.d/create_ap