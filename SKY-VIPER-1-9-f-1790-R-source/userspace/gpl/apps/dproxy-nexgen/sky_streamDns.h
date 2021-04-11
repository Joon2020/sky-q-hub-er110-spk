#ifndef SKY_STREAM_DNS_H
#define SKY_STREAM_DNS_H
/*** Interface functions exposed from Sky DNS stream implementation ****************/
int sky_handleStreamSocket(); /* Handle Sky specific operation on DNS stream handling */
int sky_cancelStreamDNSHandle();/* Cancel the Sky specific DNS stream operations */
#endif /*SKY_STREAM_DNS_H*/
