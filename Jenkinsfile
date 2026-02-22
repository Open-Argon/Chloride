// SPDX-FileCopyrightText: 2025 William Bell
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

                    sh 'git submodule update --init --recursive'
                }
            }
        }

        stage('Detect Tag') {
            steps {
                script {
                    def tag = sh(script: "git describe --tags", returnStdout: true).trim()
                    echo "Tag detected: ${tag}"

                    if (tag.toLowerCase().contains('unstable')) {
                        echo "Tag contains 'unstable' → marking build UNSTABLE"
                        currentBuild.result = 'UNSTABLE'
                    }

                    currentBuild.displayName = "#${env.BUILD_NUMBER} ${tag}"
                    env.TAG_NAME = tag
                }
            }
        }

        stage('Setup Conan') {
            steps {
                sh '''
                    apt update
                    apt install -y cmake flex python3 python3-pip python3-venv make gcc-mingw-w64 mingw-w64 ninja-build zip jq
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
        stage('Linux Build') {
            steps {
                sh '''
                    . /tmp/venv/bin/activate
                    rm -rf build CMakeCache.txt CMakeFiles
                    conan install . --build=missing
                    conan build .
                '''
            }
        }


        stage('Archive Linux') {
            steps {
                script {
                    def version = env.TAG_NAME ?: "dev"
                    env.OUTPUT_FILE = "chloride-${version}-linux-x86_64.tar.gz"
                    echo "Packaging Linux as: ${env.OUTPUT_FILE}"
                }
                sh '''
                    cp LICENSE.txt build/bin/
                    tar -czf "$OUTPUT_FILE" -C build/bin .
                '''
                archiveArtifacts artifacts: "${env.OUTPUT_FILE}", allowEmptyArchive: false
            }
        }

        stage('Windows Build') {
            steps {
                sh '''
                  . /tmp/venv/bin/activate
                  rm -rf build CMakeCache.txt CMakeFiles
                  conan install . \
                      --profile:host=mingw-x86_64.txt \
                      --build=missing
                  conan build . \
                      --profile:host=mingw-x86_64.txt

                '''
            }
        }
        stage('Archive Windows') {
            steps {
                script {
                    def version = env.TAG_NAME ?: "dev"
                    env.OUTPUT_FILE = "chloride-${version}-windows-x86_64.zip"
                    echo "Packaging Windows as: ${env.OUTPUT_FILE}"
                }
                sh '''
                    cp LICENSE.txt build/bin/
                    # Adjust packaging format if needed
                    zip -r "$OUTPUT_FILE" build/bin/*
                '''
                archiveArtifacts artifacts: "${env.OUTPUT_FILE}", allowEmptyArchive: false
            }
        }
        stage('Publish Gitea Release') {
            when {
                expression { env.TAG_NAME }
            }
            environment {
                GITEA_TOKEN = credentials('gitea-token')
            }
            steps {
                sh '''
                    set -e

                    API="$GITEA_URL/api/v1"
                    OWNER_REPO="$GITEA_REPO"
                    TAG="$TAG_NAME"

                    echo "Creating release for $TAG"

                    RELEASE_JSON=$(curl -s -X POST \
                    -H "Authorization: token $GITEA_TOKEN" \
                    -H "Content-Type: application/json" \
                    "$API/repos/$OWNER_REPO/releases" \
                    -d "{
                            \\"tag_name\\": \\"$TAG\\",
                            \\"name\\": \\"$TAG\\",
                            \\"draft\\": false,
                            \\"prerelease\\": false
                        }" || true)

                    RELEASE_ID=$(echo "$RELEASE_JSON" | jq -r '.id')

                    echo "Release ID: $RELEASE_ID"

                    for f in chloride-$TAG-*; do
                        echo "Uploading $f"
                        curl -X POST \
                        -H "Authorization: token $GITEA_TOKEN" \
                        -H "Content-Type: application/octet-stream" \
                        --data-binary @"$f" \
                        "$API/repos/$OWNER_REPO/releases/$RELEASE_ID/assets?name=$(basename $f)"
                    done
                '''
            }
        }
    }

    post {
        always {
            script {
                def tag = sh(script: "git describe --tags", returnStdout: true).trim()
                echo "Detected tag: ${tag}"

                if (tag.toLowerCase().contains("unstable")) {
                    echo "Unstable tag detected"
                    currentBuild.result = "SUCCESS"
                } else {
                    echo "Stable tagged build"
                    currentBuild.description = "Stable"
                    currentBuild.result = "SUCCESS"
                }
            }
        }
    }
}
