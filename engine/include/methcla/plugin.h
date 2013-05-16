/*
    Copyright 2012-2013 Samplecount S.L.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef METHCLA_PLUGIN_H_INCLUDED
#define METHCLA_PLUGIN_H_INCLUDED

#include <methcla/common.h>
#include <methcla/file.h>
#include <stdbool.h>
#include <stddef.h>

//* Realtime interface.
typedef struct Methcla_World Methcla_World;

//* Non-realtime interface.
typedef struct Methcla_Host Methcla_Host;

//* Synth handle managed by a plugin.
typedef void Methcla_Synth;

//* Shared resource handle managed by the realtime context.
typedef void* Methcla_Resource;

//* Callback function type for performing commands in the non-realtime context.
typedef void (*Methcla_HostPerformFunction)(const Methcla_Host* host, void* data);

//* Realtime interface
struct Methcla_World
{
    //* Handle for implementation specific data.
    void* handle;

    //* Return engine sample rate.
    double (*sampleRate)(const struct Methcla_World* world);

    // Realtime memory allocation
    void* (*alloc)(const struct Methcla_World* world, size_t size);
    void* (*allocAligned)(const struct Methcla_World* world, size_t alignment, size_t size);
    void (*free)(const struct Methcla_World* world, void* ptr);

    //* Schedule a command for execution in the non-realtime context.
    void (*performCommand)(const struct Methcla_World* world, Methcla_HostPerformFunction perform, void* data);

    //* Reference counted resources.
    void (*retain)(const struct Methcla_World* world, Methcla_Resource resource);
    void (*release)(const struct Methcla_World* world, Methcla_Resource resource);

    // This is necessary for some asynchronous command execution schemes.
    Methcla_Resource (*synth_get_resource)(const struct Methcla_World* world, Methcla_Synth* synth);
};

static inline double methcla_world_samplerate(const Methcla_World* world)
{
    return world->sampleRate(world);
}

static inline void* methcla_world_alloc(const Methcla_World* world, size_t size)
{
    return world->alloc(world, size);
}

static inline void* methcla_world_alloc_aligned(const Methcla_World* world, size_t alignment, size_t size)
{
    return world->allocAligned(world, alignment, size);
}

static inline void methcla_world_free(const Methcla_World* world, void* ptr)
{
    world->free(world, ptr);
}

static inline void methcla_world_perform_command(const Methcla_World* world, Methcla_HostPerformFunction perform, void* data)
{
    world->performCommand(world, perform, data);
}

static inline void methcla_world_retain(const Methcla_World* world, Methcla_Resource resource)
{
    return world->retain(world, resource);
}

static inline void methcla_world_release(const Methcla_World* world, Methcla_Resource resource)
{
    return world->release(world, resource);
}

static inline Methcla_Resource methcla_world_synth_get_resource(const Methcla_World* world, Methcla_Synth* synth)
{
    return world->synth_get_resource(world, synth);
}

static inline Methcla_Resource methcla_world_synth_acquire_resource(const Methcla_World* world, Methcla_Synth* synth)
{
    Methcla_Resource resource = world->synth_get_resource(world, synth);
    methcla_world_retain(world, resource);
    return resource;
}

typedef enum
{
    kMethcla_Input,
    kMethcla_Output
} Methcla_PortDirection;

typedef enum
{
    kMethcla_ControlPort,
    kMethcla_AudioPort
} Methcla_PortType;

typedef enum
{
    kMethcla_PortFlags  = 0x0
  , kMethcla_Trigger    = 0x1
} Methcla_PortFlags;

typedef struct Methcla_PortDescriptor Methcla_PortDescriptor;

struct Methcla_PortDescriptor
{
    Methcla_PortDirection direction;
    Methcla_PortType      type;
    Methcla_PortFlags     flags;
};

typedef void Methcla_SynthOptions;

typedef struct Methcla_SynthDef Methcla_SynthDef;

struct Methcla_SynthDef
{
    //* Synth definition URI.
    const char* uri;

    //* Size of an instance in bytes.
    size_t instance_size;

    //* Size of options struct in bytes.
    size_t options_size;

    //* Parse OSC options and fill options struct.
    void (*configure)(const void* tag_buffer, size_t tag_size, const void* arg_buffer, size_t arg_size, Methcla_SynthOptions* options);

    //* Get port descriptor at index.
    bool (*port_descriptor)(const Methcla_SynthOptions* options, size_t index, Methcla_PortDescriptor* port);

    //* Construct a synth instance at the location given.
    void (*construct)(const Methcla_World* world, const Methcla_SynthDef* def, const Methcla_SynthOptions* options, Methcla_Synth* synth);

    //* Connect port at index to data.
    void (*connect)(Methcla_Synth* synth, size_t index, void* data);

    //* Activate the synth instance just before starting to call `process`.
    void (*activate)(const Methcla_World* world, Methcla_Synth* synth);

    //* Process numFrames of audio samples.
    void (*process)(const Methcla_World* world, Methcla_Synth* synth, size_t numFrames);

    //* Destroy a synth instance.
    void (*destroy)(const Methcla_World* world, Methcla_Synth* synth);
};

//* Callback function type for performing commands in the realtime context.
typedef void (*Methcla_WorldPerformFunction)(const Methcla_World* world, void* data);

typedef struct Methcla_Host
{
    //* Handle for implementation specific data.
    void* handle;

    //* Register a synth definition.
    void (*registerSynthDef)(const struct Methcla_Host* host, const Methcla_SynthDef* synthDef);

    //* Lookup sound file API.
    const Methcla_SoundFileAPI* (*soundFileAPI)(const struct Methcla_Host* host, const char* mimeType);

    //* Schedule a command for execution in the realtime context.
    void (*performCommand)(const struct Methcla_Host* host, const Methcla_WorldPerformFunction perform, void* data);

    Methcla_Synth* (*resource_get_synth)(const struct Methcla_Host* host, Methcla_Resource resource);
} Methcla_Host;

static inline Methcla_Synth* methcla_host_resource_get_synth(const Methcla_Host* host, Methcla_Resource resource)
{
    return host->resource_get_synth(host, resource);
}

static inline void methcla_host_register_synthdef(const Methcla_Host* host, const Methcla_SynthDef* synthDef)
{
    if ((host == NULL) || (host->registerSynthDef == NULL) ||
        (synthDef == NULL))
        return; // TODO: return error code
    host->registerSynthDef(host, synthDef);
}

static inline Methcla_FileError methcla_host_soundfile_open(const Methcla_Host* host, const char* path, Methcla_FileMode mode, Methcla_SoundFile** file, Methcla_SoundFileInfo* info)
{
    if ((host == NULL) ||
        (host->soundFileAPI == NULL) ||
        (path == NULL) || (*path == '\0') ||
        (file == NULL) ||
        (info == NULL))
        return kMethcla_FileInvalidArgument;
    const Methcla_SoundFileAPI* api = host->soundFileAPI(host, "audio/*");
    return api == NULL ? kMethcla_FileUnspecifiedError
                       : api->open(api, path, mode, file, info);
}

static inline void methcla_host_perform_command(const Methcla_Host* host, Methcla_WorldPerformFunction perform, void* data)
{
    host->performCommand(host, perform, data);
}

typedef struct Methcla_Library
{
    //* Handle for implementation specific data.
    void* handle;

    //* Destroy the library and clean up associated resources.
    void (*destroy)(const struct Methcla_Library* library);
} Methcla_Library;

typedef const Methcla_Library* (*Methcla_LibraryFunction)(const Methcla_Host* host, const char* bundlePath);

// #define MESCALINE_MAKE_INIT_FUNC(name) MethclaInit_##name
// #define MESCALINE_INIT_FUNC(name) MESCALINE_MAKE_INIT_FUNC(name)

#endif /* METHCLA_PLUGIN_H_INCLUDED */
