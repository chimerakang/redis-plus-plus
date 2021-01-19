/* 
 * macAddress.h
 *
 * Created on 1/21/2017
 * Author: Andrew Keener
 */

#include <string>
#include <sstream>
#include <iomanip>
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <net/if.h>
#include <unistd.h>

#ifndef __MACADDRESS_H_
#define __MACADDRESS_H_

/**
 * struct macAddress - MAC Address structure
 *
 * This struct stores MAC addresses 
 *
 * @myAddress: unsigned int array storing 48bit MAC
 */ 

class macAddress 
{ 
public:

    u_int8_t mac_addr[6];

macAddress();
macAddress(uint8_t mac_byte1,
           uint8_t mac_byte2,
           uint8_t mac_byte3,
           uint8_t mac_byte4,
           uint8_t mac_byte5,
           uint8_t mac_byte6);

/** @brief Pulls MAC Address of wlan6
 *
 *  Will pull the mac address of wlan6
 */
void getMac(std::string);



/** @brief converts mac to string
 *
 *  Converts unsigned mac bytes to 
 *  ascii hex string
 */

std::string toString() const;

private:

};

macAddress::macAddress()
{
    mac_addr[0] = 0;
	mac_addr[1] = 0;
	mac_addr[2] = 0;
	mac_addr[3] = 0;
	mac_addr[4] = 0;
	mac_addr[5] = 0;
}

macAddress::macAddress( uint8_t mac_byte1,
                        uint8_t mac_byte2,
                        uint8_t mac_byte3,
                        uint8_t mac_byte4,
                        uint8_t mac_byte5,
                        uint8_t mac_byte6)
{
        mac_addr[0] = mac_byte1;
        mac_addr[1] = mac_byte2;
        mac_addr[2] = mac_byte3;
        mac_addr[3] = mac_byte4;
        mac_addr[4] = mac_byte5;
        mac_addr[5] = mac_byte6;
}

std::string macAddress::toString() const {

        std::stringstream mac_string;
        std::string mac;
        mac_string << std::hex << std::uppercase << std::setfill('0');
        for(int i = 0; i < 6; i++)
        {
                mac_string << std::setw(2) << (uint16_t)mac_addr[i] << ':';
        }
        mac = mac_string.str();
        mac.erase(mac.size()-1);
        return mac;
}


void macAddress::getMac(std::string ifrName){
        struct ifreq s;
        int fd =  socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
        strcpy(s.ifr_name, ifrName.c_str() );
        char buffer [3];
        unsigned char macStr [18];
        if(0 == ioctl(fd, SIOCGIFHWADDR, &s)) {
                mac_addr[0] = (unsigned char) s.ifr_addr.sa_data[0];
                mac_addr[1] = (unsigned char) s.ifr_addr.sa_data[1];
                mac_addr[2] = (unsigned char) s.ifr_addr.sa_data[2];
                mac_addr[3] = (unsigned char) s.ifr_addr.sa_data[3];
                mac_addr[4] = (unsigned char) s.ifr_addr.sa_data[4];
                mac_addr[5] = (unsigned char) s.ifr_addr.sa_data[5];

                }
}

#endif
