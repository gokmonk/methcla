#ifndef MESCALINE_AUDIO_ENGINE_H_INCLUDED
#define MESCALINE_AUDIO_ENGINE_H_INCLUDED

#include <Mescaline/Audio.hpp>
#include <Mescaline/Audio/AudioBus.hpp>
#include <Mescaline/Audio/CommandEngine.hpp>
#include <Mescaline/Audio/IO/Client.hpp>
#include <Mescaline/Audio/Node.hpp>
#include <Mescaline/Audio/SynthDef.hpp>
#include <Mescaline/Exception.hpp>
#include <Mescaline/Memory/Manager.hpp>

#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/unordered_map.hpp>
#include <boost/utility.hpp>

#include <bitset>
#include <string>
#include <vector>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"

namespace Mescaline { namespace Audio
{
    // class ControlBus : boost::noncopyable
    // {
    // public:
    //     typedef sample_t ValueType;
    // 
    // private:
    //     BusId       m_id;
    //     Epoch       m_epoch;
    //     ValueType   m_data;
    // };

    struct EngineException : virtual Mescaline::Exception { };
    struct InvalidNodeId : virtual EngineException { };
    struct DuplicateNodeId : virtual EngineException { };
    struct ErrorInfoNodeIdTag { };
    typedef boost::error_info<ErrorInfoNodeIdTag, NodeId> ErrorInfoNodeId;

    class Node;

    class NodeMap
    {
        typedef std::vector<Node*> Nodes;

    public:
        typedef Nodes::const_reference const_reference;

        NodeMap(size_t maxNumNodes)
            : m_nodes(maxNumNodes, 0)
        { }

        void insert(Node* node);
        const_reference lookup(const NodeId& nodeId) const { return m_nodes.at(nodeId); }
        void release(const NodeId& nodeId);

    private:
        Nodes m_nodes;
    };

    class Environment;

    class Command
    {
    public:
        Command(Environment& env, Context context)
            : m_env(env)
            , m_context(context)
        { }
        virtual ~Command() { }

        Environment& env() { return m_env; }
        Context context() const { return m_context; }

        virtual void perform(Context context) = 0;

    private:
        Environment&    m_env;
        Context         m_context;
    };

    class APIRequest : public Command
    {
    public:
        typedef boost::function1<void, const LV2_Atom*> ResponseHandler;

        APIRequest(Environment& env, LV2_Atom* atom, const ResponseHandler& handler=ResponseHandler());
        virtual ~APIRequest();

        virtual void perform(Context context);

    private:
        LV2_Atom*       m_atom;
        ResponseHandler m_responseHandler;
    };

    class RTCommand : public Command
                    , public Memory::Allocated<RTCommand, Memory::RTMemoryManager>
    {
    public:
        RTCommand(Environment& env)
            : Command(env, kRealtime)
        { }
    };

    template <class T> class DeferredDeleteCommand : public RTCommand
    {
    public:
        DeferredDeleteCommand(Environment& env, T* ptr)
            : RTCommand(env)
            , m_ptr(ptr)
        { }

        virtual void perform(Context context)
        {
            BOOST_ASSERT( context == kNonRealtime );
            delete m_ptr;
            m_ptr = 0;
            env().free(context, this);
        }

    private:
        T* m_ptr;
    };

    class Group;
    using Memory::RTMemoryManager;

    class Environment : public boost::noncopyable
    {
    public:
        struct Options
        {
            Options()
                : maxNumNodes(1024)
                , maxNumAudioBuses(128)
                , maxNumControlBuses(4096)
                , sampleRate(44100)
                , blockSize(64)
                , numHardwareInputChannels(2)
                , numHardwareOutputChannels(2)
            { }

            size_t maxNumNodes;
            size_t maxNumAudioBuses;
            size_t maxNumControlBuses;
            size_t sampleRate;
            size_t blockSize;
            size_t numHardwareInputChannels;
            size_t numHardwareOutputChannels;
        };

        Environment(Plugin::Manager& pluginManager, const Options& options);
        ~Environment();

        Plugin::Manager& pluginManager() { return m_synthDefs; }
        const Plugin::Manager::PluginHandle& lookupSynthDef(const char* name) { return m_synthDefs.lookup(name); }

        Group* rootNode() { return m_rootNode; }

        size_t sampleRate() const { return m_sampleRate; }
        size_t blockSize() const { return m_blockSize; }

        ResourceId nextResourceId() { return m_resources.nextId(); }

        AudioBus::Handle audioBus(const ResourceId& busId)
        {
            return boost::dynamic_pointer_cast<AudioBus>(m_resources.lookup(busId));
        }

//        {
//            switch (busId.scope()) {
//                case AudioBusId::kInput:
//                    return m_audioInputChannels[busId.id()];
//                case AudioBusId::kOutput:
//                    return m_audioOutputChannels[busId.id()];
//                case AudioBusId::kInternal:
//                    return m_audioBuses[busId.id()];
//            }
//            BOOST_THROW_EXCEPTION(InvalidInput());
//        }

        RTMemoryManager& rtMem() { return m_rtMem; }

        const Epoch& epoch() const { return m_epoch; }

        void process(size_t numFrames, sample_t** inputs, sample_t** outputs);

        struct Uris
        {
            LV2_URID atom_String;
        };

        // URIs and messages
        const Uris& uris() const { return m_uris; }
        const LV2_Atom_Forge& atomForge() const { return m_forge; }

        void sendRequest(LV2_Atom* msg);

        // Commands
        void enqueue(Context context, Command* cmd);
        void free(Context context, Command* cmd);

    protected:
        friend class Resource;

        void addResource(Resource& resource);
        void removeResource(Resource& resource);

    private:
        const size_t                m_sampleRate;
        const size_t                m_blockSize;
        RTMemoryManager             m_rtMem;
        Plugin::Manager&            m_synthDefs;
        ResourceMap                 m_resources;
        Group*                      m_rootNode;
        boost::ptr_vector<ExternalAudioBus> m_audioInputChannels;
        boost::ptr_vector<ExternalAudioBus> m_audioOutputChannels;
        boost::ptr_vector<InternalAudioBus> m_audioBuses;
        Epoch                       m_epoch;
        CommandChannel<Command>     m_commandChannel;
        CommandEngine<Command>      m_commandEngine;
        Uris                        m_uris;
        LV2_Atom_Forge              m_forge;
    };
    
    class Engine : public IO::Client
    {
    public:
        Engine(Plugin::Loader* pluginLoader);
        virtual ~Engine();

        virtual void configure(const IO::Driver& driver);
        virtual void process(size_t numFrames, sample_t** inputs, sample_t** outputs);
    
        Environment& env()
        {
            BOOST_ASSERT( m_env != 0 );
            return *m_env;
        }
        const Environment& env() const
        {
            BOOST_ASSERT( m_env != 0 );
            return *m_env;
        }

    private:
        Plugin::Loader*     m_pluginLoader;
        Plugin::Manager     m_pluginManager;
        Environment*        m_env;
    };
}; };

#endif // MESCALINE_AUDIO_ENGINE_H_INCLUDED
