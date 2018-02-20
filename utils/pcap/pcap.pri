win32-msvc*: {
    INCLUDEPATH += $$PWD/winpcap/include
    contains(QMAKE_TARGET.arch,x86_64): LIBS += -L$$PWD/winpcap/lib/x64
    else: LIBS += -L$$PWD/winpcap/lib
    LIBS += -lwpcap
} else: {
    LIBS += -lpcap
}

OTHER_FILES += \
    winpcap/version.info \
    winpcap/Lib/wpcap.lib \
    winpcap/Lib/Packet.lib \
    winpcap/Lib/libwpcap.a \
    winpcap/Lib/libpacket.a \
    winpcap/Lib/x64/wpcap.lib \
    winpcap/Lib/x64/Packet.lib

HEADERS += \
    winpcap/Include/Win32-Extensions.h \
    winpcap/Include/remote-ext.h \
    winpcap/Include/pcap-stdinc.h \
    winpcap/Include/pcap-namedb.h \
    winpcap/Include/pcap-bpf.h \
    winpcap/Include/pcap.h \
    winpcap/Include/Packet32.h \
    winpcap/Include/ip6_misc.h \
    winpcap/Include/bittypes.h \
    winpcap/Include/pcap/vlan.h \
    winpcap/Include/pcap/usb.h \
    winpcap/Include/pcap/sll.h \
    winpcap/Include/pcap/pcap.h \
    winpcap/Include/pcap/namedb.h \
    winpcap/Include/pcap/bpf.h \
    winpcap/Include/pcap/bluetooth.h
