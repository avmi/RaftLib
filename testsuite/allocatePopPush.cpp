/**
 * allocateSendPush.cpp -
 * @author: Jonathan Beard
 * @version: Sat Feb 27 19:10:26 2016
 *
 * Copyright 2016 Jonathan Beard
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <cstdint>
#include <cstdio>
#include <cstddef>
#include <raft>
#include <cstdlib>
#include <cassert>
#include "foodef.tcc"

using obj_t = foo< 100 >;

class start : public raft::kernel
{
public:
    start() : raft::kernel()
    {
        output.addPort< obj_t >( "y" );
    }

    virtual ~start() = default;

    virtual raft::kstatus run()
    {
        auto &mem( output[ "y" ].allocate< obj_t >() );
        for( auto i( 0 ); i < mem.length; i++ )
        {
            mem.pad[ i ] = static_cast< int >( counter );
        }
        output[ "y" ].send();
        counter++;
        if( counter == 200 )
        {
            return( raft::stop );
        }
        return( raft::proceed );
    }

private:
    std::size_t counter = 0;
};


class middle : public raft::kernel
{
public:
    middle() : raft::kernel()
    {
        input.addPort< obj_t >( "x" );
        output.addPort< obj_t >( "y" );
    }

    virtual ~middle() = default;

    virtual raft::kstatus run()
    {
        obj_t obj;
        input[ "x" ].pop( obj );
        output[ "y" ].push( obj );
        return( raft::proceed );
    }
};


class last : public raft::kernel
{
public:
    last() : raft::kernel()
    {
        input.addPort< obj_t >( "x" );
    }

    virtual ~last() = default;

    virtual raft::kstatus run()
    {
        auto &mem( input[ "x" ].peek< obj_t >() );

        using index_type = std::remove_const_t<decltype(mem.length)>;
        for( index_type i( 0 ); i < mem.length; i++ )
        {
            //will fail if we've messed something up
            assert( static_cast<std::size_t>(mem.pad[ i ]) == counter );
        }
        input[ "x" ].unpeek();
        input[ "x" ].recycle();
        counter++;
        return( raft::proceed );
    }

private:
    std::size_t counter = 0;
};

int
main()
{
    start s;
    middle m;
    last l;

    raft::map M;
    M += s >> m >> l;
    M.exe();
    return( EXIT_SUCCESS );
}
