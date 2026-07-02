pipeline {
    agent any

    environment {
        TOOLNAME="MugenFramework"
        REPO="https://github.com/MugenFramework/Mugen.git"
    }

    stages{
        stage('Cleanup'){
            steps{
                deleteDir()
                dir("${TOOLNAME}"){
                    deleteDir()
                }
            }
        }

        stage('Git Mugen'){
            steps{
                sh "git clone --single-branch --branch main ${REPO}"
            }
        }

        stage('Build - cmake'){
            steps{
                sh "mkdir -p ./Mugen/client/Build && cd ./Mugen/client/Build && cmake .."
            }
        }

        stage('Build - compile'){
            steps{
                sh "cmake --build ./Mugen/client/Build -- -j4"
            }
        }

        stage('Sanity-Check'){
            steps{
                sh 'file ./Mugen/client/Mugen'
            }
        }
    }
}
