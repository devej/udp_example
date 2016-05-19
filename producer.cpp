//
// producer.cpp
//

#include "stdio.h"
#include <boost/asio.hpp>
#include <string>

using namespace boost::asio;



class UDPSender
{
public:
    UDPSender( boost::asio::io_service& io_service, const std::string& ip, const std::string& port ) :
        io_service_(io_service), socket_(io_service, ip::udp::endpoint(ip::udp::v4(), 0))
    {
        ip::udp::resolver this_resolver( io_service );
        ip::udp::resolver::query qry( ip::udp::v4(), ip, port );
        ip::udp::resolver::iterator iter = this_resolver.resolve( qry );
        endpoint_ = *iter;
    }

    ~UDPSender() {
        socket_.close();
    }
    
    size_t send( const std::string& data )
    {
        return socket_.send_to( boost::asio::buffer(data, data.size()), endpoint_ );
    }

private:
    io_service&         io_service_;
    ip::udp::socket     socket_;
    ip::udp::endpoint   endpoint_;
};



int main( int argc, char* argv[] )
{
    if( argc != 2 ) {
        fprintf( stderr, "Too many arguments.  Just provide a filename please.\n" );
        return 1;
    }


    // Open the file, read it line by line, sending each line as-is.
    FILE* file = fopen( argv[1], "r" );
    if( !file ) {
        fprintf( stderr, "Unable to open \"%s\".\n", argv[1] );
        return 1;
    }


    boost::asio::io_service io_service;
    io_service.run();
    UDPSender sender( io_service, "localhost", "20033" );

    char readBuffer[ 1024 ] = {0};
    unsigned long recordsRead = 0;

    while( fgets( readBuffer, 1024, file ) )
    {
        ++recordsRead;              // count the # of rows we read
        sender.send( readBuffer );  // send the readBuffer
    }

    fclose( file );
    file = NULL;

    // Send a "Transmission Complete" message to signal the consumer we're finished.
    sender.send( "ENDTRANSMISSION" );

    io_service.stop();
    fprintf( stdout, "Total rows: %d\n", recordsRead );
    return 0;
}

