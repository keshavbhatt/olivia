![Olivia Banner Art](https://dashboard.snapcraft.io/site_media/appmedia/2019/03/banner_BPmKykd.png)

# Olivia - Elegant music player for LINUX 

being developed in BANKUBAD LABS by @keshavbhatt

[![Snap Status](https://build.snapcraft.io/badge/keshavbhatt/olivia.svg)](https://build.snapcraft.io/user/keshavbhatt/olivia)

﻿**Nightly Build can be installed using:**

[![Get it from the Snap Store](https://snapcraft.io/static/images/badges/en/snap-store-black.svg)](https://snapcraft.io/olivia-test)

**If you are having issues in desktop theming like big fonts and weird cursor theme**

Run Olivia with the following command:

    QT_STYLE_OVERRIDE='gtk' olivia.test.olivia

**Features**
* Allows search music online
* Allows organize music 
* Allows download song while streaming
* Allows search youtube and add result to library
* Plays audio only of youtube streams (saves data bandwidth)
* Support themes , Dynamic theme based on album art
* Search suggestions
* Player mini mode included , minimal player widget with always on capability and allows set transparency.
* Internet radio, allows play more than 25k online radio stations, list them sort them according to language and country
* Top music chart, allows list top 100 songs country wise
* More features like cloud synchronisation of music using an online account coming soon


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

    qmake
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
