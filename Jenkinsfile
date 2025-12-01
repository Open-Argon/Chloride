pipeline {
    agent any
    environment {
        GITEA_URL = 'https://git.wbell.dev'
        GITEA_REPO = 'Open-Argon/Chloride'
    }
    stages {
        stage('Checkout') {
          steps {
          script {
            // Detect if this is a tag build via Jenkins-supplied vars
            if (env.GIT_TAG) {
              echo "Checking out tag: ${env.GIT_TAG}"
              checkout([
                $class: 'GitSCM',
                branches: [[name: "refs/tags/${env.GIT_TAG}"]],
                userRemoteConfigs: [[url: scm.userRemoteConfigs[0].url]],
                doGenerateSubmoduleConfigurations: false,
                extensions: [
                  [$class: 'SubmoduleUpdate', recursiveSubmodules: true]
                ]
              ])
            } else {
              echo "Checking out normal branch"
              checkout scm
            }

            // update submodules
            sh '''
              git fetch --tags
              latest_tag=$(git describe --tags --abbrev=0)
              git checkout tags/$latest_tag
            '''
            sh 'git submodule update --init --recursive'
          }
        }
      }


        stage('Detect Tag') {
      steps {
        script {
          echo "REF from Gitea: ${env.GITEA_REF}"
          echo "GIT_BRANCH: ${env.GIT_BRANCH}"
          echo "GIT_TAG: ${env.GIT_TAG}"

          def ref = sh(script: "git rev-parse --symbolic-full-name HEAD", returnStdout: true).trim()
          if (ref.startsWith('refs/tags/')) {
            def tag = ref.replace('refs/tags/', '')
            echo "Tag detected: ${tag}"

            if (tag.toLowerCase().contains('unsable')) {
              echo "Tag contains 'unsable' → marking build UNSTABLE"
              currentBuild.result = 'UNSTABLE'
            }

            // Expose for other stages
            env.TAG_NAME = tag
          } else {
            // Normal branch push = DEV build
            def branchName = ref.replace("refs/heads/", "")
            echo "Regular branch build: ${branchName}"

            // Mark display name as a dev build
            currentBuild.displayName = "#${env.BUILD_NUMBER} DEV-${branchName}"
          }
        }
      }
        }

        stage('Setup Conan') {
      steps {
        sh '''
                            apt update
                            apt install -y cmake flex python3 python3-pip python3-venv
                            python3 -m venv /tmp/venv
                            . /tmp/venv/bin/activate
                            pip install --upgrade pip
                            pip install conan
                        '''
      }
        }

        stage('Setup Conan Profile') {
      steps {
        sh '''
                            . /tmp/venv/bin/activate
                            rm -rf ~/.conan2
                            conan profile detect
                        '''
      }
        }

        stage('Install Dependencies') {
      steps {
        sh '''
                            . /tmp/venv/bin/activate
                            conan install . --build=missing
                        '''
      }
        }

        stage('Build Project') {
      steps {
        sh '''
                            . /tmp/venv/bin/activate
                            conan build .
                        '''
      }
        }


stage('Archive Build Artifacts') {
      steps {
          script {
              // Determine platform
              def os = sh(returnStdout: true, script: 'uname -s').trim().toLowerCase()
              def arch = sh(returnStdout: true, script: 'uname -m').trim().toLowerCase()

              // Determine version (tag or "dev")
              def version = env.TAG_NAME ?: "dev"

              // Construct file name
              env.OUTPUT_FILE = "chloride-${version}-${os}-${arch}.tar.gz"

              echo "Packaging as: ${env.OUTPUT_FILE}"
          }

          sh '''
              # Ensure LICENSE.txt is in the output directory
              cp LICENSE.txt build/bin/

              # Create tarball with auto-generated name
              tar -czf "$OUTPUT_FILE" -C build/bin .
          '''

          archiveArtifacts artifacts: "${env.OUTPUT_FILE}", allowEmptyArchive: false
      }
    }
  }
  
  post {
    always {
        script {
            // Automatically detects full ref name
            def ref = sh(script: "git rev-parse --symbolic-full-name HEAD", returnStdout: true).trim()

            if (ref.startsWith("refs/tags/")) {
                // Extract tag name
                def tag = ref.replace("refs/tags/", "")
                echo "Detected tag: ${tag}"

                if (tag.toLowerCase().contains("unstable")) {
                    echo "Unstable tag detected"
                    currentBuild.result = "UNSTABLE"
                } else {
                    echo "Stable tagged build"
                    currentBuild.description = "Stable"
                    currentBuild.result = "SUCCESS"
                }

            } else {
                currentBuild.result = "UNSTABLE"
                echo "Regular commit → marking as dev build"
                currentBuild.description = "Dev Build"
            }
        }
    }
  }
}
