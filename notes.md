ssh -vvv -i ~/.ssh/hetzner01 deploy@157.90.114.127
pass: Oxekopp123!


# 42_IRC Deployment Guide

This document provides a step-by-step guide to deploying the 42_IRC server on a VPS. It covers installing dependencies, uploading the project, building the server, configuring the firewall, and setting up a systemd service for easy management.

todo:

- implementieren /kick --> Matthias
- implementieren /topic --> Daniel 
- topic neu setzen nur, wenn der channel das erlaubt (mode -t --> sonst nur operatoren erlaubt) --> Matthias
- implementieren /mode --> Daniel
- implementieren /invite --> Daniel
- sanitzing channel namen - &/# sind als erstes Zeichen Bedingung
- /JOIN erweitern? Befehl auf mehrere argumente erweitern? /join #channel1,#channel2,#channel3 --> Matthias
- /MODE string plus parameters? mode k --> siehe nächster punkt
- /KEY pro channel?
- Dazu REGEX??? Oder lieber über c++ (Daniel: c++)
- Livetest mit allen 1h





