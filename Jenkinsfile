def tascar_build_steps(stage_name) {
    // Extract components from stage_name:
    def system, arch, devenv
    (system,arch,devenv) = stage_name.split(/ *&& */) // regexp for missing/extra spaces

    // checkout tascarpro from version control system, the exact same revision that
    // triggered this job on each build slave
    checkout scm

    // Avoid that artifacts from previous builds influence this build
    sh "git reset --hard && git clean -ffdx"

    // get tags from public repo:
    sh "git fetch --tags https://github.com/HoerTech-gGmbH/tascar"

    if (system != "windows") {
        // Autodetect libs/compiler on Unix
        sh "make pack"
        // Store debian packets for later retrieval by the repository manager
        stash name: (arch+"_"+system), includes: 'packaging/deb/tascar/'
        archiveArtifacts 'packaging/deb/tascar/**'
    } else {
        // Compile subset of TASCAR on Windows
        sh "make -j 4 libtascar googletest"
        sh "make -j 4 -C libtascar unit-tests"
        sh "make -C apps build/tascar_cli"
        sh "make -j 4 -C gui build/tascar_spkcalib build/tascar"
        sh "make -C plugins build/.directory"
        sh("make -C plugins -j 4 build/tascarsource_omni.dll" +
           " build/tascar_ap_pink.dll build/tascar_ap_sndfile.dll" +
           " build/tascar_ap_spksim.dll build/tascar_oscjacktime.dll" + 
           " build/tascar_system.dll build/tascarreceiver_hoa2d.dll" +
           " build/tascarreceiver_hrtf.dll build/tascarreceiver_nsp.dll" +
           " build/tascarreceiver_omni.dll build/tascar_hrirconv.dll" +
           " build/tascarreceiver_foaconv.dll")
        sh "mkdir -p TASCAR"
        // Collect all compiled artifacts
        sh "cp -v */build/[lt]*.{dll,exe} TASCAR"
        // Add all DLLs dependencies provided by Msys2
        dir('TASCAR') {
            sh 'cp -v $(cygcheck ./* | grep mingw64 | sort -bu) .'
            sh "mkdir -p share/{icons,glib-2.0} lib"
        }
        sh "cp -r /mingw64/share/icons/Adwaita TASCAR/share/icons"
        sh "cp -r /mingw64/share/glib-2.0/schemas TASCAR/share/glib-2.0"
        sh "cp -r /mingw64/lib/gdk-pixbuf-2.0 TASCAR/lib"
        sh "cp /mingw64/bin/gdbus.exe TASCAR"
        sh "cp artwork/*.png TASCAR"
        // Now create the zip file from everything inside the TASCAR dir
        sh 'zip -r TASCAR-$(grep ^VERSION= config.mk | cut -d= -f2)-windows.zip TASCAR'
        archiveArtifacts 'TASCAR-*-windows.zip'
    }
}

pipeline {
    agent any
    options {
        buildDiscarder(logRotator(daysToKeepStr: '7', artifactDaysToKeepStr: '7'))
    }
    stages {
        stage("build") {
            parallel {
                stage(                        "focal && x86_64 && tascardev") {
                    agent {label              "focal && x86_64 && tascardev"}
                    steps {tascar_build_steps("focal && x86_64 && tascardev")}
                }
                stage(                        "bionic && x86_64 && tascardev") {
                    agent {label              "bionic && x86_64 && tascardev"}
                    steps {tascar_build_steps("bionic && x86_64 && tascardev")}
                }
                stage(                        "bionic && armv7 && tascardev") {
                    agent {label              "bionic && armv7 && tascardev"}
                    steps {tascar_build_steps("bionic && armv7 && tascardev")}
                }
                stage(                        "windows && x86_64 && tascardev") {
                    agent {label              "windows && x86_64 && tascardev"}
                    steps {tascar_build_steps("windows && x86_64 && tascardev")}
                }
            }
        }
        stage("artifacts") {
            agent {label "aptly"}
            // do not publish packages for any branches except these
            when { anyOf { branch 'master'; branch 'development' } }
            steps {
                // receive all deb packages from tascarpro build
                unstash "x86_64_focal"
                unstash "x86_64_bionic"
                unstash "armv7_bionic"

                // Copies the new debs to the stash of existing debs,
                sh "make -f hoertechstorage.mk storage"

                build job: "/Packaging/hoertech-aptly/$BRANCH_NAME", quietPeriod: 300, wait: false
            }
        }
    }
    post {
        failure {
            mail to: 'grimm@hz-ol.de,herzke@hz-ol.de',
            subject: "Failed Pipeline: ${currentBuild.fullDisplayName}",
            body: "Something is wrong with ${env.BUILD_URL} ($GIT_URL)"
        }
    }
}
