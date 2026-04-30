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

        stage('Archive Source') {
            steps {
                script {
                    def version = env.TAG_NAME ?: "dev"
                    env.OUTPUT_FILE = "archives/chloride-source-${version}.tar.gz"
                    echo "Packaging Source as: ${env.OUTPUT_FILE}"
                }
                sh '''
                mkdir -p archives
                git ls-files --recurse-submodules | tar -czf $OUTPUT_FILE -T -
                '''
                archiveArtifacts artifacts: "${env.OUTPUT_FILE}", allowEmptyArchive: false, fingerprint: true
            }
        }

        stage('Setup Conan') {
            steps {
                sh '''
                    apt update
                    apt install -y cmake flex python3 python3-pip python3-venv make gcc-mingw-w64 mingw-w64 ninja-build zip jq gh dpkg-dev
                    python3 -m venv /tmp/venv
                    . /tmp/venv/bin/activate
                    pip install --upgrade pip
                    pip install conan

                    mkdir -p archives macos-artifacts
                    rm -rf archives/* macos-artifacts/* *.zip *.tar.gz
                '''
            }
        }

        stage('Build (Parallel)') {
            parallel {

                stage('Linux Build') {
                    environment {
                        CONAN_HOME = "${WORKSPACE}/.conan-linux"
                    }
                    stages {
                        stage('Build') {
                            steps {
                                sh '''
                                    . /tmp/venv/bin/activate
                                    rm -rf out/linux $CONAN_HOME
                                    conan profile detect
                                    conan install . --build=missing -of "out/linux"
                                    conan build . -of "out/linux"

                                    cp -r stdlib out/linux/build/dist/
                                    ./build-stdlib.sh out/linux/build/dist/stdlib -j ARGON_INCLUDE="$(realpath include)"
                                '''
                            }
                        }
                    }
                }

                stage('Windows Build') {
                    environment {
                        CONAN_HOME = "${WORKSPACE}/.conan-windows"
                    }
                    stages {
                        stage('Build') {
                            steps {
                                sh '''
                                    . /tmp/venv/bin/activate
                                    rm -rf out/windows $CONAN_HOME
                                    conan profile detect
                                    conan install . \
                                        --profile:host=mingw-x86_64.txt \
                                        --build=missing -of "out/windows"
                                    conan build . \
                                        --profile:host=mingw-x86_64.txt -of "out/windows"

                                    cp -r stdlib out/windows/build/dist/
                                    ./build-stdlib.sh out/windows/build/dist/stdlib -j TARGET_OS=windows ARGON_INCLUDE="$(realpath include)"
                                '''
                            }
                        }
                    }
                }

                stage('macOS Build (GitHub Actions)') {
                    environment {
                        GH_TOKEN = credentials('github-pat')
                        GH_REPO  = 'open-argon/chloride'
                        WORKFLOW = 'macOS Build (Jenkins-triggered)'
                        BUILD_NAME_ARG = "${env.TAG_NAME ?: 'dev'}"
                    }
                    steps {
                        sh '''
                            set -e

                            # Decide what ref to build
                            REF=$(git describe --tags --exact-match 2>/dev/null || git rev-parse HEAD)
                            echo "Triggering macOS build for ref: $REF"

                            # Trigger workflow
                            gh workflow run "$WORKFLOW" \
                                --repo "$GH_REPO" \
                                --ref main \
                                -f ref="$REF" \
                                -f build_name="$BUILD_NAME_ARG"

                            # Get the latest run ID
                            RUN_ID=$(gh run list \
                                --repo "$GH_REPO" \
                                --workflow "$WORKFLOW" \
                                --limit 1 \
                                --json databaseId \
                                -q '.[0].databaseId')

                            echo "Waiting for GitHub Actions run $RUN_ID"
                            gh run watch "$RUN_ID" --repo "$GH_REPO"

                            # Download artifact
                            gh run download "$RUN_ID" \
                                --repo "$GH_REPO" \
                                --name macos-build \
                                --dir macos-artifacts
                        '''
                    }
                }
            }
        }


        stage('Archive Linux') {
            steps {
                script {
                    def version = env.TAG_NAME ?: "dev"
                    env.OUTPUT_FILE = "archives/argon-${version}-linux-x86_64.tar.gz"
                    echo "Packaging Linux as: ${env.OUTPUT_FILE}"
                }
                sh '''
                    cp LICENSE.txt out/linux/build/dist/
                    cp -r LICENSES out/linux/build/dist/
                    tar -czf "$OUTPUT_FILE" -C out/linux/build/dist .
                '''
                archiveArtifacts artifacts: "${env.OUTPUT_FILE}", allowEmptyArchive: false, fingerprint: true
            }
        }

        stage('Debian Package Build') {
            steps {
                script {
                    def version = env.TAG_NAME ?: "0.0.0-1"
                    env.DEB_VERSION = version.replaceFirst('^v', '')  // strip leading 'v'
                    env.OUTPUT_FILE = "archives/argon-${env.DEB_VERSION}-x86_64.deb"
                    env.PACKAGE_ROOT = "${env.WORKSPACE}/argon-${env.DEB_VERSION}-x86_64"
                }
                sh '''
                    set -e
                    INSTALL_INTERNAL="/usr/local/lib/chloride"

                    rm -rf "$PACKAGE_ROOT"

                    DESTDIR="$PACKAGE_ROOT" cmake --install out/linux/build --prefix "$INSTALL_INTERNAL"

                    mkdir -p "$PACKAGE_ROOT$INSTALL_INTERNAL/stdlib"
                    cp -R stdlib/* "$PACKAGE_ROOT$INSTALL_INTERNAL/stdlib/"

                    mkdir -p "$PACKAGE_ROOT/usr/bin"
                    printf '#!/bin/bash\nexec "%s/bin/argon" "$@"\n' "$INSTALL_INTERNAL" \
                        > "$PACKAGE_ROOT/usr/bin/argon"
                    chmod +x "$PACKAGE_ROOT/usr/bin/argon"

                    mkdir -p "$PACKAGE_ROOT/DEBIAN"
                    printf 'Package: argon\nVersion: %s\nArchitecture: amd64\nMaintainer: Ugric\nDescription: Interpreter written in C for the argon programming language\n' \
                        "$DEB_VERSION" > "$PACKAGE_ROOT/DEBIAN/control"

                    dpkg-deb --build "$PACKAGE_ROOT" "$OUTPUT_FILE"
                '''
                archiveArtifacts artifacts: "${env.OUTPUT_FILE}", allowEmptyArchive: false, fingerprint: true
            }
        }

        stage('Archive Windows') {
            steps {
                script {
                    def version = env.TAG_NAME ?: "dev"
                    env.OUTPUT_FILE = "archives/argon-${version}-windows-x86_64.zip"
                    echo "Packaging Windows as: ${env.OUTPUT_FILE}"
                }
                sh '''
                    cp LICENSE.txt out/windows/build/dist/
                    cp -r LICENSES out/windows/build/dist/
                    
                    (
                    cd "out/windows/build/dist" && zip -r "../../../../$OUTPUT_FILE" .
                    )
                '''
                archiveArtifacts artifacts: "${env.OUTPUT_FILE}", allowEmptyArchive: false, fingerprint: true
            }
        }

        stage('Archive macOS') {
            steps {
                archiveArtifacts artifacts: 'macos-artifacts/**/*.tar.gz', fingerprint: true
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
