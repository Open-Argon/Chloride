pipeline {
    agent any
    stages {
      stage('Checkout') {
          steps {
            checkout scm
            sh 'git submodule update --init --recursive'
          }
      }

      stage('Setup Conan') {
          steps {
            sh '''
                        python3 -m venv /tmp/venv
                        . /tmp/venv/bin/activate
                        apt update
                        apt upgrade -y
                        apt install -y cmake flex python3 python3-pip python3-venv
                        pip install --upgrade pip
                        pip install conan
                    '''
          }
      }

      stage('Setup Conan Profile') {
          steps {
            sh '''
                        . /tmp/venv/bin/activate
                        if [ ! -f ~/.conan2/profiles/default ]; then
                            conan profile detect
                        fi
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
          sh '''
              # Copy LICENSE.txt into build/bin
              cp LICENSE.txt build/bin/

              # Create tarball with the contents of build/bin at the root
              tar -czf chloride.tar.gz -C build/bin .
          '''
          // Archive the tarball
          archiveArtifacts artifacts: 'chloride.tar.gz', allowEmptyArchive: false
        }
      }
    }
}
