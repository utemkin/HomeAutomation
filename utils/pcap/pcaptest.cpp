#include <qdatetime.h>
#ifdef Q_OS_WIN
#define HAVE_REMOTE
#endif
#include "pcap.h"

void iftest(pcap_if_t *d)
{
    char errbuf[PCAP_ERRBUF_SIZE+1];

    pcap_t *adhandle;
#ifdef Q_OS_WIN
    if((adhandle=pcap_open(d->name,         // name of the device
                           1518,            // portion of the packet to capture.
                           PCAP_OPENFLAG_NOCAPTURE_LOCAL,
                           1,               // read timeout
                           NULL,            // auth
                           errbuf           // error buffer
                           ))==NULL)
#else
    if((adhandle=pcap_open_live(d->name,    // name of the device
                                1518,		// portion of the packet to capture.
                                0,			// promiscuous mode (nonzero means promiscuous)
                                1,  		// read timeout
                                errbuf		// error buffer
                                ))==NULL)
#endif
    {
        printf("Error in pcap_open_live: %s\n",errbuf);
        return;
    }

    int* dlt_buf;
    int dlt_size;
    if((dlt_size=pcap_list_datalinks(adhandle,&dlt_buf))==-1)
    {
        printf("Error in pcap_list_datalinks: %s\n",pcap_geterr(adhandle));
        pcap_close(adhandle);
        return;
    }

    for(int i=0;i<dlt_size;++i)
    {
        printf("Datalink %i %s %s\n",dlt_buf[i],pcap_datalink_val_to_name(dlt_buf[i]),
            pcap_datalink_val_to_description(dlt_buf[i]));
    }

    pcap_free_datalinks(dlt_buf);
    pcap_close(adhandle);

}

int main(int argc,char** argv)
{
    printf("%s\n",pcap_lib_version());

    char errbuf[PCAP_ERRBUF_SIZE+1];

    if(argc<2)
    {
        pcap_if_t* alldevs;
        if(pcap_findalldevs(&alldevs,errbuf)==-1)
        {
            printf("Error in pcap_findalldevs: %s\n",errbuf);
            return 1;
        }

        for(pcap_if_t* d=alldevs;d;d=d->next)
        {
            printf("Name: %s\n",d->name);
            printf("Description: %s\n",d->description);
            printf("Loopback: %s\n",(d->flags & PCAP_IF_LOOPBACK)?"yes":"no");
            iftest(d);
            printf("\n");
        }

        pcap_freealldevs(alldevs);
    }
    else
    {
        pcap_t *adhandle;
#ifdef Q_OS_WIN
        if((adhandle=pcap_open(argv[1],         // name of the device
                               1518,            // portion of the packet to capture.
                               PCAP_OPENFLAG_NOCAPTURE_LOCAL,
                               1,               // read timeout
                               NULL,            // auth
                               errbuf           // error buffer
                               ))==NULL)
#else
        if((adhandle=pcap_open_live(argv[1],    // name of the device
                                    1518,		// portion of the packet to capture.
                                    0,			// promiscuous mode (nonzero means promiscuous)
                                    1,  		// read timeout
                                    errbuf		// error buffer
                                    ))==NULL)
#endif
        {
            printf("Error in pcap_open_live: %s\n",errbuf);
            return 2;
        }

        if(pcap_set_datalink(adhandle,DLT_EN10MB)==-1)
        {
            printf("Error in pcap_set_datalink: %s\n",pcap_geterr(adhandle));
            pcap_close(adhandle);
            return 3;
        }

#if 1
        bpf_program fcode;
        if(pcap_compile(adhandle,&fcode,"(ether proto 0x866)||(ip proto 187)",1,0)<0)
        {
            printf("Error in pcap_compile: %s\n",pcap_geterr(adhandle));
            pcap_close(adhandle);
            return 4;
        }
        if(pcap_setfilter(adhandle,&fcode)<0)
        {
            printf("Error in pcap_setfilter: %s\n",pcap_geterr(adhandle));
            pcap_close(adhandle);
            return 5;
        }
        pcap_freecode(&fcode);
#endif

        pcap_pkthdr* header;
        const u_char* pkt_data;
        int res;

        qint64 last_send=QDateTime::currentMSecsSinceEpoch();
        while((res=pcap_next_ex(adhandle,&header,&pkt_data))>=0)
        {
            if(last_send+1000<QDateTime::currentMSecsSinceEpoch())
            {
                last_send+=1000;
                u_char data[]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x8,0x66};
                if(pcap_sendpacket(adhandle,data,sizeof(data))!=0)
                {
                    printf("Error in pcap_sendpacket: %s\n",pcap_geterr(adhandle));
                }
            }
            if(res==0)
            {
                continue;
            }
            printf("len=%u caplen=%u\n",header->len,header->caplen);
            if(header->caplen<14)
            {
                continue;
            }
            printf("dst MAC=%02x:%02x:%02x:%02x:%02x:%02x src MAC=%02x:%02x:%02x:%02x:%02x:%02x lenght/type=%04x\n",
                   pkt_data[0],pkt_data[1],pkt_data[2],pkt_data[3],pkt_data[4],pkt_data[5],
                   pkt_data[6],pkt_data[7],pkt_data[8],pkt_data[9],pkt_data[10],pkt_data[11],
                   (pkt_data[12]<<8)|pkt_data[13]);
            for(bpf_u_int32 i=0;i<header->caplen-14;++i)
            {
                printf("%02x ",pkt_data[i+14]);
                if((i&0xf)==0xf||i==header->caplen-14-1)
                {
                    printf("\n");
                }
            }
        }
        printf("Error in pcap_next_ex: %s\n",pcap_geterr(adhandle));

        pcap_close(adhandle);
    }

    return 0;
}
