/*
    Copyright (C) 2021  Aaron Feng

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    My Github homepage: https://github.com/AaronFeng753
*/
#include "mainwindow.h"
#include "ui_mainwindow.h"

/*
视频补帧 (IFRNet)
*/
bool MainWindow::IFRNet_FrameInterpolation(QString SourcePath,QString OutputPath)
{
    if(SourcePath.right(1)=="/")
    {
        SourcePath = SourcePath.left(SourcePath.length() - 1);
    }
    if(OutputPath.right(1)=="/")
    {
        OutputPath = OutputPath.left(OutputPath.length() - 1);
    }
    if(file_isDirExist(SourcePath)==false)return false;
    //=======
    emit Send_TextBrowser_NewMessage(tr("Starting to interpolate frames (IFRNet) in:[")+SourcePath+"]");
    //==== 检测是否启用了自动调整线程数量,若启用则强制设定重试次数大于6 ====
    int retry_add = 0;
    if(ui->checkBox_AutoAdjustNumOfThreads_VFI->isChecked()==true && ui->spinBox_retry->value()<6 && ui->checkBox_MultiThread_VFI->isChecked())
    {
        retry_add = 6-ui->spinBox_retry->value();
    }
    //==== 检查是否需要启用uhd模式 (IFRNet不直接支持UHD参数，但检查大分辨率输入) ====
    bool isUhdInput=false;
    if(ui->checkBox_UHD_VFI->isChecked()==false)
    {
        QStringList SourceImagesNames = file_getFileNames_in_Folder_nofilter(SourcePath);
        QString ImgName_tmp;
        QMap<QString,int> res_map;
        int original_height;
        int original_width;
        for(int i=0; i<SourceImagesNames.size(); i++)
        {
            ImgName_tmp = SourceImagesNames.at(i);
            QFileInfo ImgName_tmp_info(ImgName_tmp);
            if(ImgName_tmp_info.suffix()=="png")
            {
                res_map = Image_Gif_Read_Resolution(SourcePath+"/"+ImgName_tmp);
                original_height = res_map["height"];
                original_width = res_map["width"];
                if(original_height>0 && original_width>0)//判断是否读取失败
                {
                    isUhdInput = ((original_height*original_width)>=8294400);
                    break;
                }
            }
        }
    }
    if(isUhdInput==true)Send_TextBrowser_NewMessage(tr("UHD input detected, UHD Mode is automatically enabled."));
    //====================
    int FileNum_MAX = file_getFileNames_in_Folder_nofilter(SourcePath).size() * ui->spinBox_MultipleOfFPS_VFI->value();
    int FileNum_New = 0;
    int FileNum_Old = 0;
    //========
    QString FrameInterpolation_ProgramPath = Current_Path+"/ifrnet-ncnn-vulkan/ifrnet-ncnn-vulkan_waifu2xEX.exe";
    QString CMD ="";
    //========
    bool FrameInterpolation_QProcess_failed = false;
    QString ErrorMSG="";
    QString StanderMSG="";
    int FrameNumDigits = CalNumDigits(FileNum_MAX);
    //========
    int MultiFPS_MAX = ui->spinBox_MultipleOfFPS_VFI->value();
    int MultiFPS_Init = 2;
    //初始化进度讯息
    bool is_progressBar_CurrentFile_available = false;
    if(ui->checkBox_ShowInterPro->isChecked()==true && ui->progressBar_CurrentFile->isVisible()==false)
    {
        is_progressBar_CurrentFile_available=true;
        emit Send_CurrentFileProgress_Start(SourcePath.split("/").last(),FileNum_MAX);
    }
    //========
    for(int MultiFPS_curr = MultiFPS_Init; MultiFPS_curr<=MultiFPS_MAX; MultiFPS_curr*=2)
    {
        bool isThisRoundSucceed = false;
        QString OutputPath_Curr = "";
        QString SourcePath_Curr = "";
        if(MultiFPS_curr == MultiFPS_Init)
        {
            SourcePath_Curr = SourcePath;
        }
        else
        {
            SourcePath_Curr = OutputPath+"_"+QString::number(MultiFPS_curr/2);
        }
        if(MultiFPS_curr == MultiFPS_MAX)
        {
            OutputPath_Curr = OutputPath;
        }
        else
        {
            OutputPath_Curr = OutputPath+"_"+QString::number(MultiFPS_curr);
        }
        file_DelDir(OutputPath_Curr);
        file_mkDir(OutputPath_Curr);
        //=======
        for(int retry=0; retry<(ui->spinBox_retry->value()+retry_add); retry++)
        {
            FrameInterpolation_QProcess_failed = false;
            ErrorMSG="";
            StanderMSG="";
            //=====
            QProcess FrameInterpolation_QProcess;
            CMD ="\""+FrameInterpolation_ProgramPath+"\" -i \""+SourcePath_Curr+"\" -o \""+OutputPath_Curr+"\" -f %0"+QString("%1").arg(FrameNumDigits)+"d.png"+IFRNet_ReadConfig(isUhdInput,FileNum_MAX);
            FrameInterpolation_QProcess.start(CMD);
            while(!FrameInterpolation_QProcess.waitForStarted(200)&&!QProcess_stop) {}
            while(!FrameInterpolation_QProcess.waitForFinished(200)&&!QProcess_stop)
            {
                if(waifu2x_STOP)
                {
                    FrameInterpolation_QProcess.close();
                    file_DelDir(OutputPath_Curr);
                    if(SourcePath_Curr != SourcePath)file_DelDir(SourcePath_Curr);
                    if(is_progressBar_CurrentFile_available)
                    {
                        emit Send_CurrentFileProgress_Stop();//停止当前文件进度条
                    }
                    return false;
                }
                //=========
                ErrorMSG.append(FrameInterpolation_QProcess.readAllStandardError().toLower());
                StanderMSG.append(FrameInterpolation_QProcess.readAllStandardOutput().toLower());
                if(ErrorMSG.contains("failed")||StanderMSG.contains("failed"))
                {
                    FrameInterpolation_QProcess_failed = true;
                    FrameInterpolation_QProcess.close();
                    file_DelDir(OutputPath_Curr);
                    break;
                }
                //=========
                if(ui->checkBox_ShowInterPro->isChecked())
                {
                    FileNum_New = file_getFileNames_in_Folder_nofilter(OutputPath_Curr).size();
                    if(FileNum_New!=FileNum_Old)
                    {
                        if(is_progressBar_CurrentFile_available==false)
                        {
                            emit Send_TextBrowser_NewMessage(tr("Interpolating frames (IFRNet) in:[")+SourcePath_Curr+tr("] Progress:[")+QString::number(FileNum_New,10)+"/"+QString::number(FileNum_MAX,10)+"]");
                        }
                        else
                        {
                            emit Send_CurrentFileProgress_progressbar_SetFinishedValue(FileNum_New);
                        }
                        FileNum_Old=FileNum_New;
                    }
                }
            }
            if(FrameInterpolation_QProcess_failed==false)
            {
                ErrorMSG.append(FrameInterpolation_QProcess.readAllStandardError().toLower());
                StanderMSG.append(FrameInterpolation_QProcess.readAllStandardOutput().toLower());
                if(ErrorMSG.contains("failed")||StanderMSG.contains("failed"))
                {
                    FrameInterpolation_QProcess_failed = true;
                    file_DelDir(OutputPath_Curr);
                }
            }
            //========= 检测是否成功,是否需要重试 ============
            if(FrameInterpolation_QProcess_failed==false && (file_getFileNames_in_Folder_nofilter(SourcePath_Curr).size() * MultiFPS_Init == file_getFileNames_in_Folder_nofilter(OutputPath_Curr).size()))
            {
                isThisRoundSucceed = true;
                break;
            }
            else
            {
                file_DelDir(OutputPath_Curr);
                //===
                if(retry==(ui->spinBox_retry->value()+retry_add-1))
                {
                    break;
                }
                //===
                if(retry>=2 && ui->checkBox_AutoAdjustNumOfThreads_VFI->isChecked()==true)
                {
                    isSuccessiveFailuresDetected_VFI=true;
                }
                //===
                file_mkDir(OutputPath_Curr);
                emit Send_TextBrowser_NewMessage(tr("Automatic retry, please wait."));
                Delay_sec_sleep(5);
            }
        }
        if(isThisRoundSucceed==true)
        {
            if(MultiFPS_curr == MultiFPS_MAX)
            {
                if(SourcePath_Curr != SourcePath)file_DelDir(SourcePath_Curr);
                emit Send_TextBrowser_NewMessage(tr("Finish interpolating frames (IFRNet) in:[")+SourcePath+"]");
                if(is_progressBar_CurrentFile_available)
                {
                    emit Send_CurrentFileProgress_Stop();//停止当前文件进度条
                }
                return true;
            }
            else
            {
                if(SourcePath_Curr != SourcePath)file_DelDir(SourcePath_Curr);
            }
        }
        else
        {
            emit Send_TextBrowser_NewMessage(tr("Failed to interpolate frames (IFRNet) in:[")+SourcePath+"]");
            //=======
            if(is_progressBar_CurrentFile_available)
            {
                emit Send_CurrentFileProgress_Stop();//停止当前文件进度条
            }
            //========
            return false;
        }
    }
    //=======
    emit Send_TextBrowser_NewMessage(tr("Failed to interpolate frames (IFRNet) in:[")+SourcePath+"]");
    if(is_progressBar_CurrentFile_available)
    {
        emit Send_CurrentFileProgress_Stop();//停止当前文件进度条
    }
    //=======
    return false;
}

QString MainWindow::IFRNet_ReadConfig(bool isUhdInput,int NumOfFrames)
{
    QString VFI_Config = " ";
    //TTA
    if(ui->checkBox_TTA_VFI->isChecked())
    {
        VFI_Config.append("-x ");
    }
    //UHD
    if(ui->checkBox_UHD_VFI->isChecked() || isUhdInput)
    {
        VFI_Config.append("-u ");
    }
    //GPU & 多线程
    int NumOfThreads_VFI = 1;
    if(isSuccessiveFailuresDetected_VFI==false)
    {
        NumOfThreads_VFI = ui->spinBox_NumOfThreads_VFI->value();
    }
    if(ui->checkBox_MultiGPU_VFI->isChecked()==false)
    {
        //单显卡
        if(ui->comboBox_GPUID_VFI->currentText().trimmed().toLower()!="auto")
        {
            VFI_Config.append("-g "+ui->comboBox_GPUID_VFI->currentText().trimmed()+" ");
        }
        QString jobs_num_str = QString("%1").arg(NumOfThreads_VFI);
        VFI_Config.append(QString("-j "+jobs_num_str+":"+jobs_num_str+":"+jobs_num_str+" "));
    }
    else
    {
        //多显卡
        QString GPU_IDs_str = ui->lineEdit_MultiGPU_IDs_VFI->text().trimmed().trimmed().replace("，",",").remove(" ").remove("　");
        if(GPU_IDs_str.right(1)==",")
        {
            GPU_IDs_str = GPU_IDs_str.left(GPU_IDs_str.length() - 1);
        }
        QStringList GPU_IDs_StrList = GPU_IDs_str.split(",");
        GPU_IDs_StrList.removeAll("");
        GPU_IDs_StrList.removeDuplicates();
        if(GPU_IDs_StrList.isEmpty())
        {
            QString jobs_num_str = QString("%1").arg(NumOfThreads_VFI);
            VFI_Config.append(QString("-j "+jobs_num_str+":"+jobs_num_str+":"+jobs_num_str+" "));
        }
        else
        {
            GPU_IDs_str = "";
            for(int i=0; i<GPU_IDs_StrList.size(); i++)
            {
                if(i==0)
                {
                    GPU_IDs_str.append(GPU_IDs_StrList.at(i));
                }
                else
                {
                    GPU_IDs_str.append(","+GPU_IDs_StrList.at(i));
                }
            }
            VFI_Config.append("-g "+GPU_IDs_str+" ");
            int NumOfThreads_AVG = NumOfThreads_VFI / GPU_IDs_StrList.size();
            if(NumOfThreads_AVG<1)NumOfThreads_AVG=1;
            int NumOfThreads_Total = NumOfThreads_AVG * GPU_IDs_StrList.size();
            QString NumOfThreads_AVG_str = QString("%1").arg(NumOfThreads_AVG);
            QString NumOfThreads_Total_str = QString("%1").arg(NumOfThreads_Total);
            QString Jobs_Str = "";
            for(int i=0; i<GPU_IDs_StrList.size(); i++)
            {
                if(i==0)
                {
                    Jobs_Str.append(NumOfThreads_AVG_str);
                }
                else
                {
                    Jobs_Str.append(","+NumOfThreads_AVG_str);
                }
            }
            VFI_Config.append(QString("-j "+NumOfThreads_Total_str+":"+Jobs_Str+":"+NumOfThreads_Total_str+" "));
        }
    }
    //Model
    QString engine_folder = Current_Path + "/ifrnet-ncnn-vulkan";
    VFI_Config.append("-m \""+engine_folder+"/"+ui->comboBox_Model_IFRNet->currentText().trimmed()+"\" ");
    //========================
    return VFI_Config;
}

/*
================================================================================
                    IFRNet NCNN VULKAN 检测可用GPU
=================================================================================
*/
void MainWindow::on_pushButton_DetectGPU_IFRNet_clicked()
{
    //====
    ui->pushButton_DetectGPU_IFRNet->setText(tr("Detecting, please wait..."));
    //====
    pushButton_Start_setEnabled_self(0);
    ui->comboBox_GPUID_VFI->setEnabled(0);
    ui->pushButton_DetectGPU_VFI->setEnabled(0);
    ui->pushButton_DetectGPU_IFRNet->setEnabled(0);
    Available_GPUID_IFRNet.clear();
    QtConcurrent::run(this, &MainWindow::IFRNet_DetectGPU);
}

int MainWindow::IFRNet_DetectGPU()
{
    emit Send_TextBrowser_NewMessage(tr("Detecting available GPU for IFRNet, please wait."));
    //===============
    QString InputPath = Current_Path + "/Compatibility_Test/Compatibility_Test.jpg";
    QString InputPath_1 = Current_Path + "/Compatibility_Test/Compatibility_Test_1.jpg";
    QString OutputPath = Current_Path + "/Compatibility_Test/res.png";
    QFile::remove(OutputPath);
    //==============
    QString program = Current_Path+"/ifrnet-ncnn-vulkan/ifrnet-ncnn-vulkan_waifu2xEX.exe";
    QString model_path = Current_Path+"/ifrnet-ncnn-vulkan/IFRNet_Vimeo90K";
    //=========
    int GPU_ID=-1;
    //=========
    while(true)
    {
        QFile::remove(OutputPath);
        QProcess *Waifu2x = new QProcess();
        QString gpu_str = " -g "+QString::number(GPU_ID,10)+" ";
        QString cmd = "\"" + program + "\"" + " -0 " + "\"" + InputPath + "\"" + " -1 " + "\"" + InputPath_1 + "\" -o " + "\"" + OutputPath + "\"" + " -j 1:1:1 " + gpu_str + " -m \""+model_path+"\"";
        Waifu2x->start(cmd);
        while(!Waifu2x->waitForStarted(100)&&!QProcess_stop) {}
        while(!Waifu2x->waitForFinished(100)&&!QProcess_stop) {}
        if(QFile::exists(OutputPath) && (Waifu2x->readAllStandardError().toLower().contains("failed")||Waifu2x->readAllStandardOutput().toLower().contains("failed"))==false)
        {
            Available_GPUID_IFRNet.append(QString::number(GPU_ID,10));
            GPU_ID++;
            QFile::remove(OutputPath);
        }
        else
        {
            if(GPU_ID > -1)
            {
                break;
            }
            else
            {
                GPU_ID++;
            }
        }
    }
    QFile::remove(OutputPath);
    //===============
    emit Send_TextBrowser_NewMessage(tr("Detection is complete!"));
    if(Available_GPUID_IFRNet.isEmpty())
    {
        Send_TextBrowser_NewMessage(tr("No available GPU ID detected!"));
    }
    emit Send_IFRNet_DetectGPU_finished();
    return 0;
}

int MainWindow::IFRNet_DetectGPU_finished()
{
    pushButton_Start_setEnabled_self(1);
    ui->pushButton_DetectGPU_VFI->setEnabled(1);
    ui->pushButton_DetectGPU_IFRNet->setEnabled(1);
    if(ui->checkBox_MultiGPU_VFI->isChecked()==false)ui->comboBox_GPUID_VFI->setEnabled(1);
    //====
    ui->comboBox_GPUID_VFI->clear();
    ui->comboBox_GPUID_VFI->addItem("auto");
    if(!Available_GPUID_IFRNet.isEmpty())
    {
        QString AvaIDs_Str=tr("Available GPU IDs for IFRNet Frame Interpolation engine:[");
        for(int i=0; i<Available_GPUID_IFRNet.size(); i++)
        {
            ui->comboBox_GPUID_VFI->addItem(Available_GPUID_IFRNet.at(i));
            if(i == (Available_GPUID_IFRNet.size()-1))
            {
                AvaIDs_Str.append(Available_GPUID_IFRNet.at(i));
            }
            else
            {
                AvaIDs_Str.append(Available_GPUID_IFRNet.at(i)+",");
            }
        }
        AvaIDs_Str.append("]");
        emit Send_TextBrowser_NewMessage(AvaIDs_Str);
    }
    //====
    ui->pushButton_DetectGPU_IFRNet->setText(tr("Detect available GPU ID (IFRNet)"));
    //====
    return 0;
}

/*
IFRNet 兼容性检测
*/
void MainWindow::on_checkBox_isCompatible_IFRNetNcnnVulkan_clicked()
{
    ui->checkBox_isCompatible_IFRNetNcnnVulkan->setChecked(isCompatible_IFRNetNcnnVulkan);
}
