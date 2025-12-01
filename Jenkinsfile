pipeline {
    agent any
    environment {
        GITEA_URL = 'https://git.wbell.dev'
        GITEA_REPO = 'Open-Argon/Chloride'
    }
    stages {
        stage('Checkout') {
      steps {
        checkout scm
        sh 'git submodule update --init --recursive'
      }
        }

        stage('Detect Tag') {
      steps {
        script {
          if (env.GIT_BRANCH?.startsWith('refs/tags/')) {
            def tag = env.GIT_BRANCH.replace('refs/tags/', '')
            echo "Tag detected: ${tag}"

            if (tag.toLowerCase().contains('unsable')) {
              echo "Tag contains 'unsable' → marking build UNSTABLE"
              currentBuild.result = 'UNSTABLE'
            }

            // Expose for other stages
            env.TAG_NAME = tag
          } else {
            // Normal branch push = DEV build
            def branchName = env.GIT_BRANCH.replace("refs/heads/", "")
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
            sh '''
                # Copy LICENSE.txt into build/bin
                cp LICENSE.txt build/bin/

                # Create tarball with the contents of build/bin at the root
                tar -czf chloride-linux-x86_64.tar.gz -C build/bin .
            '''
            // Archive the tarball
            archiveArtifacts artifacts: 'chloride-linux-x86_64.tar.gz', allowEmptyArchive: false
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
