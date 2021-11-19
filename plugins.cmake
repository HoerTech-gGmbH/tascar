if (NOT RECEIVERS)
    list(APPEND RECEIVERS
            amb1h0v
            amb1h1v
            amb3h0v
            amb3h3v
            cardioid
            cardioidmod
            chmap
            debugpos
            fakebf
            foaconv
            hann
            hoa2d
            hoa2d_fuma
            hoa2d_fuma_hos
            hoa3d
            hoa3d_enc
            hrtf
            nsp
            omni
            ortf
            simplefdn
            vmic
            wfs
            )
endif ()

if (NOT TASCARMODS)
    list(APPEND TASCARMODS
            #artnetdmx
            #circularfence
            #datalogging
            dirgain
            dummy
            epicycles
            fail
            #fence
            foa2hoadiff
            #glabsensors
            granularsynth
            hoafdnrot
            hossustain
            hrirconv
            jackrec
            #joystick
            #levels2osc
            #lightctl
            locationmodulator
            locationvelocity
            #lslactor
            #lsljacktime
            #ltcgen
            matrix
            #midicc2osc
            #midictl
            motionpath
            mpu6050track
            nearsensor
            #opendmxusb
            orientationmodulator
            oscevents
            oscjacktime
            #oscrelay
            #oscserver
            pendulum
            #pos2lsl
            pos2osc
            #qualisystracker
            rotator
            route
            sampler
            savegains
            sleep
            system
            touchosc
            waitforjackport
            )
endif ()

if(NOT SOURCEMODS)
        list(APPEND SOURCEMODS
                cardioidmod
                door
                generic1storder
                omni
        )
endif()

if (NOT AUDIOPLUGINS)
    list(APPEND AUDIOPLUGINS
            delay
            dummy
            gate
            hannenv
            identity lipsync
            lipsync_paper
            lookatme
            metronome
            onsetdetector
            sine
            sndfile
            spksim
            gainramp
            pulse pink
            dumplevels
            feedbackdelay
            bandpass
            filter
            level2osc
            const
            noise
            loopmachine
            pos2osc
            gain
            sndfileasync
            addchannel
            )
endif ()

if (NOT MASKPLUGINS)
  list(APPEND MASKPLUGINS
    fig8
    )
endif ()

if (GTK3_FOUND AND NOT TASCARMODSGUI)
    message("Building also GTK3 related tascar GUI mods")
    list(APPEND TASCARMODSGUI
            tracegui
            simplecontroller
            timedisplay
            geopresets
            transportgui
            lightcolorpicker
            )
    list(APPEND TASCARMOD
            ${TASCARMODSGUI}
            )
endif ()
