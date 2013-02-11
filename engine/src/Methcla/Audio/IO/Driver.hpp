// Copyright 2012-2013 Samplecount S.L.
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef METHCLA_AUDIO_IO_DRIVER_HPP_INCLUDED
#define METHCLA_AUDIO_IO_DRIVER_HPP_INCLUDED

#include <Methcla/Exception.hpp>
#include <boost/cstdint.hpp>

namespace Methcla { namespace Audio { namespace IO
{
struct Exception : virtual Methcla::Exception { };

class Client;

class Driver
{
public:
    Driver(Client* client)
        : m_client(client)
    { }
    virtual ~Driver()
    { }

    Client* client() { return m_client; }

    virtual double sampleRate() const = 0;
    virtual size_t numInputs() const = 0;
    virtual size_t numOutputs() const = 0;
    virtual size_t bufferSize() const = 0;

    virtual void start() = 0;
    virtual void stop() = 0;

private:
    Client* m_client;
};

}; }; };

#endif // METHCLA_AUDIO_IO_DRIVER_HPP_INCLUDED