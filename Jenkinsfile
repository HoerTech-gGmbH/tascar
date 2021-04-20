def tascar_build_steps(stage_name) {
    // Extract components from stage_name:
    def system, arch, devenv
    (system,arch,devenv) = stage_name.split(/ *&& */) // regexp for missing/extra spaces

    // checkout tascarpro from version control system, the exact same revision that
    // triggered this job on each build slave
    checkout scm

    // Avoid that artifacts from previous builds influence this build
    sh "git reset --hard && git clean -ffdx"

    // fetch tags for correct build number calculation:
    sh "git fetch --tags"

    // Autodetect libs/compiler
    sh "make pack"

    // Store debian packets for later retrieval by the repository manager
    stash name: (arch+"_"+system), includes: 'packaging/deb/tascar/'
}

pipeline {
    agent {label "jenkinsmaster"}
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

                build job: "/hoertech-aptly/$BRANCH_NAME", quietPeriod: 300, wait: false
            }
        }
    }
    post {
        failure {
            mail to: 'g.grimm@hoertech.de,t.herzke@hoertech.de',
            subject: "Failed Pipeline: ${currentBuild.fullDisplayName}",
            body: "Something is wrong with ${env.BUILD_URL} ($GIT_URL)"
        }
    }
}
