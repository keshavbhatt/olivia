
![Olivia Banner Art](https://dashboard.snapcraft.io/site_media/appmedia/2019/03/banner_BPmKykd.png)

# Olivia - Elegant Music Player for Linux Desktop

by [@keshavbhatt](https://github.com/keshavbhatt) of [ktechpit.com](http://ktechpit.com) and [others](https://github.com/keshavbhatt/olivia/graphs/contributors)
﻿
﻿**Olivia hits mark of 5000+ active users already** 

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT) [![Average time to resolve an issue](http://isitmaintained.com/badge/resolution/keshavbhatt/olivia.svg)](http://isitmaintained.com/project/keshavbhatt/olivia "Average time to resolve an issue") [![Percentage of issues still open](http://isitmaintained.com/badge/open/keshavbhatt/olivia.svg)](http://isitmaintained.com/project/keshavbhatt/olivia "Percentage of issues still open") 

﻿**Stable and Nightly Build on any [snapd](https://docs.snapcraft.io/installing-snapd) enabled Linux Distribution can be installed using:**

﻿[![olivia](https://snapcraft.io//olivia/badge.svg)](https://snapcraft.io/olivia) [![olivia](https://snapcraft.io//olivia/trending.svg?name=0)](https://snapcraft.io/olivia)

[![Get it from the Snap Store](https://snapcraft.io/static/images/badges/en/snap-store-black.svg)](https://snapcraft.io/olivia)

    snap install olivia

**Arch Linux (using AUR):**

[Olivia Arch Linux User's Repository ](https://aur.archlinux.org/packages/olivia)  

**Features**
-   Olivia helps you discover new music and videos like no other service do.
-   Smart Music recommendation, can get you songs related to any song.
-   Allows organise music and videos at one place.
-   Olivia can save track while you are streaming it, this saves your bandwidth.
-   Olivia never stops playing song for you with its smart playlist feature it automatically starts playing related songs for you once your playlist ends.
-   Watch or Download Video for any song in different audio video formats and quality.
-   Allows search YouTube and add result to library, sort results and all other YouTube features like browse channels.
-   Plays audio only of YouTube streams (saves data bandwidth).
-   Support themes , Dynamic theme based on album art.
-   Intelligent Music Search suggestions engine integrated.
-   Player Mini mode aminimal player widget with always on capability and allows set transparency and make it act like a desktop widget.
-   Switch to Smart mode and sit back, olivia will play songs for you automatically.
-   Internet radio, allows play more than 25k online radio stations, list them sort them according to most played , most voted, language wise, country wise and by tags .
-   Olivia lets you browse new music according to your location.
-   Olivia lets you discover music based on their genres, moods and more.
-   Its easy to discover new music - singles, albums etc easily at one place.
-   Top music chart, allows list top 100 songs country wise.
-   Top albums chart, allows list top 100 albums county wise.
-   Beautiful Client side Decoration.
-   Lyrics of playing songs and separate lyrics search.
-   Powerful audio equalizers and audio filters.
-   MPRIS protocol support.
-   Audio export with meta tags and album art.
-   More features like cloud synchronisation of your Liked Music is coming soon, so you will be able to get your music back no matter where you are.

﻿**Consider Donating if you want this music player grow further**

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://paypal.me/keshavnrj/5)

[![Buy Me A Coffee](https://bmc-cdn.nyc3.digitaloceanspaces.com/BMC-button-images/custom_images/orange_img.png)](https://www.buymeacoffee.com/keshavnrj)

**Olivia utilises power of the following tools and technologies :**
- Qt GUI Framework 5.5.1 
- Bash, wget, socat, tee and other utilities that comes with "coreutils" package
- MPV Player
- Youtube-dl
- LibTag
- C++11
- Python
- Lua
- PHP, HTML, CSS, JS, JSON 
- snapcraft.io/build for continuous build and delivery through snapcraft.io/store for all major Linux distributions supporting [snapd](https://snapcraft.io/docs/installing-snapd)
- "Arch User Repository" for distribution of app to Arch Linux users.  

﻿**Build requirement**

    Qt >=5.5.1 with these modules
        - libqt5sql5-sqlite
        - libqt5webkit5 (must)
        - libqt5x11extras5
        
    mpv >= 0.29.1
    coreutils >=8.25
    socat >=1.7.3.1-1
    python >=2.7
    wget >=1.17.1
    libtag1-dev
    
**Build instructions**
With all build requirements in place go to project root and execute:

Build:

    qmake (or qmake-qt5, depending on your distro)
    make
    
Execute :

    ./olivia
     
**Screenshots:** (can be old)
![Olivia](https://dashboard.snapcraft.io/site_media/appmedia/2019/03/olivia_linux_ubuntu_1.jpeg)
![Youtube plugin for Olivia on the play](https://dashboard.snapcraft.io/site_media/appmedia/2019/03/olivia_linux_ubuntu_2.jpeg)
![Olivia Playing Internet radio](https://dashboard.snapcraft.io/site_media/appmedia/2019/03/olivia_linux_ubuntu_3.jpeg)
