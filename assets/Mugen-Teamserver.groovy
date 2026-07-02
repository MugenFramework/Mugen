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

        stage('Install-MUSL C compiler'){
            steps{
                sh "pwd && ls"
                sh "cd ./Mugen/teamserver/ && chmod +x ./Install.sh && ./Install.sh"
            }
        }

        stage('Build'){
            steps{
                sh "pwd && ls"
                sh "cd ./Mugen/teamserver/ && make"
            }
        }

        stage('Sanity-Check'){
            steps{
                sh 'file ./Mugen/teamserver/mugen'
            }
        }

    }
}
