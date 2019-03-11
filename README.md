# olivia
Elegant music player for LINUX 
not ready yet
being developed in BANKUBAD LABS by @keshavbhatt

[![Snap Status](https://build.snapcraft.io/badge/keshavbhatt/olivia.svg)](https://build.snapcraft.io/user/keshavbhatt/olivia)

[![Get it from the Snap Store](https://snapcraft.io/static/images/badges/en/snap-store-black.svg)](https://snapcraft.io/olivia-test)

﻿**Build requirement**

    Qt >=5.5.1 with these modules
        - libqt5sql5-sqlite
        - libqt5webkit5 (must)
        - libqt5x11extras5
        
    mpv >=git-2019-02-24-5370069
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
