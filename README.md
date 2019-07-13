![Olivia Banner Art](https://dashboard.snapcraft.io/site_media/appmedia/2019/03/banner_BPmKykd.png)

# Olivia - Elegant music player for LINUX 

being developed in BANKUBAD LABS by [@keshavbhatt](https://github.com/keshavbhatt)

[![Snap Status](https://build.snapcraft.io/badge/keshavbhatt/olivia.svg)](https://build.snapcraft.io/user/keshavbhatt/olivia) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT) [![Average time to resolve an issue](http://isitmaintained.com/badge/resolution/keshavbhatt/olivia.svg)](http://isitmaintained.com/project/keshavbhatt/olivia "Average time to resolve an issue") [![Percentage of issues still open](http://isitmaintained.com/badge/open/keshavbhatt/olivia.svg)](http://isitmaintained.com/project/keshavbhatt/olivia "Percentage of issues still open") 

﻿**Nightly Build on any [snapd](https://docs.snapcraft.io/installing-snapd) enabled Linux Distribution can be installed using:**

[![Get it from the Snap Store](https://snapcraft.io/static/images/badges/en/snap-store-black.svg)](https://snapcraft.io/olivia-test)

**Arch Linux ([AUR](https://aur.archlinux.org/packages/olivia/)):**

![Arch Linux User's Repository ](http://badge.kloud51.com/aur/v/olivia.svg)  ![Maintainer](http://badge.kloud51.com/aur/m/olivia.svg) ![Popularity](http://badge.kloud51.com/aur/p/olivia.svg)

**If you are having issues in desktop theming like big fonts and weird cursor theme**

Run Olivia with the following command:

    QT_STYLE_OVERRIDE='gtk' olivia-test.olivia

**Features**
* Allows search music online
* Smart Music recommendation, can get you songs related to ant particular song 
* Allows organize music 
* Allows download song while streaming
* Allows search YouTube and add result to library, sort results and other YouTube features
* Plays audio only of YouTube streams (saves data bandwidth)
* Support themes , Dynamic theme based on album art
* Search suggestions
* Player mini mode included , minimal player widget with always on capability and allows set transparency.
* Internet radio, allows play more than 25k online radio stations, list them sort them according to language and country
* Top music chart, allows list top 100 songs country wise
* Top albums chart, allows list top 100 albums county wise
* Beautiful Client side Decoration
* Lyrics of playing songs and seperate lyrics search
* Powerful audio equalizers and audio filters.
* Watch video of any song you want in your preferred audio and video quality
* More features like cloud synchronization of music using an online account coming soon

﻿**Consider Donating if you want this music player grow further**

[**PayPal Me**](https://paypal.me/keshavnrj/10)
[**Become a Patreon**](https://www.patreon.com/keshavnrj/)

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
    
**Build instructions**
With all build requirements in place go to project root and execute:

Build:

    qmake (or qmake-qt5, depending on your distro)
    make
    
Execute :

    ./olivia
     
﻿
﻿**Or build a snap package**
Copy snap directory from project root and paste it somewhere else (so the build will not mess with source code)
Run :

    snapcraft
Try snap with :

    snap try
Install snap with

    snap install --dangerous name_of_snap_file

**ScreenShots:**
![Olivia](https://dashboard.snapcraft.io/site_media/appmedia/2019/03/olivia_linux_ubuntu_1.jpeg)
![Youtube plugin for Olivia on the play](https://dashboard.snapcraft.io/site_media/appmedia/2019/03/olivia_linux_ubuntu_2.jpeg)
![Olivia Playing Internet radio](https://dashboard.snapcraft.io/site_media/appmedia/2019/03/olivia_linux_ubuntu_3.jpeg)
![Album view Olivia](https://dashboard.snapcraft.io/site_media/appmedia/2019/03/olvia_linux_ubuntu_keshav_bhatt_4.jpeg)

