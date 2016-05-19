//
// consumer.cpp
//

#include "RecordCollector.h"

#include <vector>
#include <array>
#include <string>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

using boost::asio::ip::udp;


//
// utility function
//
void split( const std::string &s, char delim, std::vector< std::string >& elems )
{
    std::stringstream ss(s);
    std::string item;
    while( std::getline(ss, item, delim) )
    {
        elems.push_back( item );
    }
}


class RecordCollector
{
public:
    void InsertRecord( const PriceRecord& pr ) {
        records_.insert( pr );
    }
    
    void WriteRecords()
    {
        std::string lastSym = "";
        uint64_t count = 0;
        FILE* file = NULL;

        for( const auto& rec : records_ )
        {
            const std::string sym = rec.GetSymbol();
            if( sym != lastSym )	// close the old file & open a new one
            {
                if( file )
                {
                    fprintf( stdout, "%s Rows: %d\n", lastSym.c_str(), count );
                    fclose( file );
                    file = NULL;
                    count = 0;
                }
                std::string fname = sym + ".csv";
                file = fopen( fname.c_str(), "w" );
                lastSym = sym;
            }

            ++count;
            fprintf( file, "%.2lf,%d,%d\n", rec.GetPrice(), rec.GetQuantity(), rec.GetSequence() );
        }

        // don't forget to write the last symbol and its count
        if( file )
        {
            fprintf( stdout, "%s Rows: %d\n", lastSym.c_str(), count );
            fclose( file );
            file = NULL;
        }
    }

private:
    std::set< PriceRecord > records_;
};



class UDPListener
{
public:
    UDPListener( boost::asio::io_service& io_service, const std::string& ip, uint32_t port ) :
        io_service_(io_service),
        socket_(
            io_service,
            udp::endpoint( boost::asio::ip::udp::v4(), port )
        )
    {
        socket_.async_receive_from(
            boost::asio::buffer(recv_buffer_, recv_buffer_.size()), endpoint_,
                boost::bind( &UDPListener::OnReceive, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred ) );
        
        listening_ = true;
    }

    ~UDPListener() {
        socket_.close();
    }

    bool IsListening() { return listening_; }

    void OnReceive( const boost::system::error_code& error, std::size_t bytes_received )
    {
        if( 0 >= bytes_received )
            return;
        
        if( !error )
        {
            recv_buffer_[ bytes_received ] = '\0';  // drop in  a null terminator to be safe
            std::string data( recv_buffer_.data() );

            if( data == "ENDTRANSMISSION" ) {
                Stop();
                return;
            }

            collector_.InsertRecord( process_received_data(data) );
            
            // keep listening
            socket_.async_receive_from(
                boost::asio::buffer(recv_buffer_, recv_buffer_.size()), endpoint_,
                    boost::bind( &UDPListener::OnReceive, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred ) );
	}
    }
    
    void Stop() {
        socket_.close();
        listening_ = false;
        collector_.WriteRecords();
    }
        
private:
    RecordCollector             collector_;
    boost::asio::io_service&    io_service_;
    udp::socket                 socket_;
    udp::endpoint               endpoint_;
    std::array<char, 1024>      recv_buffer_;
    bool                        listening_;
    
    PriceRecord process_received_data( const std::string& data )
    {
        std::vector< std::string > elements;
        split( data, ',', elements );
        if( 4 != elements.size() ) {
            throw std::logic_error( "Wrong number of elements" );
        }

        // sample data
        //0x000003ef,MSFT,9.52,3700
        //0x000003f0,AAPL,4.6,2300

        //const unsigned long seq = std::stoul( elements[0], nullptr, 16 );
        //const std::string sym = elements[1];
        //const double price = std::stod( elements[2], nullptr );
        //const unsigned long qty = std::stoul( elements[3], nullptr, 10 );

        return {
            std::stoul( elements[3], nullptr, 10 ),
            std::stoul( elements[0], nullptr, 16 ),
            std::stod( elements[2], nullptr ),
            elements[1] };
    }
};


int main( int argc, char* argv[] )
{
    // listen to udp
    // create & save records until transmission complete
    // sort and print records and summary
    boost::asio::io_service io_service;
    UDPListener listener( io_service, "localhost", 20033 );
    io_service.run();

    while( listener.IsListening() ) {
        boost::this_thread::sleep_for( boost::chrono::milliseconds(500) );
    }

    io_service.stop();

    return 0;
}
