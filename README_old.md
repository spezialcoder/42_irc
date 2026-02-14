# 42_irc
42 Project ft_irc.

## Installation on vhost

# 1. SSH into your VPS
ssh root@your-vps-ip

# 2. Install dependencies
sudo apt update
sudo apt install g++ make git

# 3. Clone your repo or upload files
git clone <your-repo>

# 4. Build
cd 42_IRC
make

# 5. Open firewall port
sudo ufw allow 7850/tcp

# 6. Run server (use screen/tmux to keep it running)
screen -S irc
./ircserv
# Press Ctrl+A then D to detach
