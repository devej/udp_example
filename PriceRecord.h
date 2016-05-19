//
//  PriceRecord
//
#pragma once

#include <string>


class PriceRecord
{
public:

    PriceRecord( uint64_t Qty, uint64_t SeqNum, double Price, const std::string& Symbol ) :
        qty_( Qty ), seq_( SeqNum ), price_( Price ), sym_( Symbol )
    { }

    ~PriceRecord() { }

    std::string ToString() const
    {
        char buf[ 1024 ] = {0};
        sprintf( buf, "%s,%.2lf,%d,%d", sym_.c_str(), price_, qty_, seq_ );
        return buf;
    }

    bool operator < ( const PriceRecord& rhs ) const
    {
        // sort by sym, price, seq
        if( sym_ != rhs.sym_ )
            return sym_ < rhs.sym_;

        if( price_ != rhs.price_ )
            return price_ < rhs.price_;

        return seq_ > rhs.seq_;
    }

    // individual getters
    std::string     GetSymbol()     const   { return sym_; }
    uint64_t        GetQuantity()   const   { return qty_; }
    uint64_t        GetSequence()   const   { return seq_; }
    double          GetPrice()      const   { return price_; }

private:
    uint64_t       qty_;
    uint64_t       seq_;
    double         price_;
    std::string    sym_;
};	// PriceRecord

