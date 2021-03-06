#include "csfwindow.h"
#include "extexecutableswidget.h"

#include <iostream>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QFileInfo>
#include <QProcess>
#include <QMessageBox>
#include <QDebug>
#include <QtCore>
#include <QScrollBar>
#include <QRegExp>

#ifndef Auto_EACSF_TITLE
#define Auto_EACSF_TITLE "Auto_EACSF"
#endif

#ifndef Auto_EACSF_CONTRIBUTORS
#define Auto_EACSF_CONTRIBUTORS "Han Bit Yoon, Arthur Le Maout"
#endif

#ifndef Auto_EACSF_VERSION
#define Auto_EACSF_VERSION ""
#endif

using std::endl;
using std::cout;

const QString CSFWindow::m_github_url = "https://github.com/ArthurLeMaout/auto_EACSF";

CSFWindow::CSFWindow(QWidget *m_parent):
    QMainWindow(m_parent)
{
    this->setupUi(this);
    checkBox_VentricleRemoval->setChecked(true);
    label_dataAlignmentMessage->setVisible(false);
    listWidget_SSAtlases->setSelectionMode(QAbstractItemView::NoSelection);
    prc= new QProcess;
    check_tree_type();
    m_data_found=find_data_dir_path();
    if(!m_data_found)
    {
        infoMsgBox(QString("Data folder not found, project building or installation might be incomplete."),QMessageBox::Warning);
    }

    readDefaultConfig();

    if (!executables.keys().isEmpty())
    {
        find_executables();
        QBoxLayout* exe_layout = new QBoxLayout(QBoxLayout::LeftToRight, tab_executables);
        ExtExecutablesWidget *exeWidget = new ExtExecutablesWidget();
        exeWidget->buildInterface(executables);
        exeWidget->setExeDir(QDir::currentPath());
        exe_layout->addWidget(exeWidget,Qt::AlignCenter);
        connect(exeWidget, SIGNAL(newExePath(QString,QString)), this, SLOT(updateExecutables(QString,QString)));
    }
    tabWidget->removeTab(1);
    tabWidget->removeTab(5);
}

CSFWindow::~CSFWindow()
{
}

bool CSFWindow::find_data_dir_path()
{
    QDir CD = QDir::current();
    CD.cdUp();
    switch(m_tree_type)
    {
        case TreeType::dev_superbuild:
            m_data_dir_path=QDir::cleanPath(CD.absolutePath()+QString("/data"));
            return (QDir(m_data_dir_path).exists());
            break;
        case TreeType::release_superbuild:
            CD.cdUp();
            m_data_dir_path=QDir::cleanPath(CD.absolutePath()+QString("/data"));
            return (QDir(m_data_dir_path).exists());
            break;
        case TreeType::install:
            m_data_dir_path=QDir::cleanPath(CD.absolutePath()+QString("/data"));
            return (QDir(m_data_dir_path).exists());
            break;
        default:
            return false;
    }
}

void CSFWindow::check_tree_type()
{
    QDir CD=QDir::current();
    CD.cdUp();
    QString CMakeFiles_path = QDir::cleanPath(CD.absolutePath()+QString("/CMakeFiles"));
    if (QDir(CMakeFiles_path).exists())
    {
        m_tree_type=TreeType::dev_superbuild;
    }
    else
    {
        CD.cdUp();
        CMakeFiles_path = QDir::cleanPath(CD.absolutePath()+QString("/CMakeFiles"));
        if (QDir(CMakeFiles_path).exists())
        {
            m_tree_type=TreeType::release_superbuild;
        }
        else{
            m_tree_type=TreeType::install;
        }
    }
}

void CSFWindow::check_exe_in_folder(QString name, QString path)
{
    QFileInfo check_exe(path);
    if (check_exe.exists() && check_exe.isExecutable())
    {
        executables[name]=path;
    }
    else
    {
        executables[name]=QString("");
    }
}

void CSFWindow::find_executables(){
    QString env_PATH(qgetenv("PATH"));
    /*
    cout<<"**************************************"<<endl;
    cout<<"VAR PATH : "<<env_PATH.toStdString()<<endl;
    cout<<"**************************************"<<endl;
    */
    QStringList path_split=env_PATH.split(":");
#ifdef Q_OS_LINUX
    QDir CD=QDir::current();

    switch (m_tree_type)
    {
        case TreeType::dev_superbuild: // If Auto_EACSF has not been installed, look in superbuild tree
            CD.cdUp(); //auto_EACSF-bin
            for (QString exe_name : executables.keys())
            {
                QString dirpath = QDir::cleanPath(CD.absolutePath()+executables[exe_name]+exe_name);
                check_exe_in_folder(exe_name,dirpath);
            }
            break;
        case TreeType::release_superbuild:
            CD.cdUp(); //auto_EACSF-build/auto_EACSF-inner-build/
            CD.cdUp(); //auto_EACSF-build/
            for (QString exe_name : executables.keys())
            {
                QString dirpath = QDir::cleanPath(CD.absolutePath()+executables[exe_name]+exe_name);
                check_exe_in_folder(exe_name,dirpath);
            }
            break;
        case TreeType::install: // If it has been installed, look in install tree
            for (QString exe_name : executables.keys())
            {
                QString dirpath = QDir::cleanPath(CD.absolutePath()+QString("/")+exe_name);
                check_exe_in_folder(exe_name,dirpath);
            }
            break;
        default:
            break;
    }




#endif
#ifdef Q_OS_MACOS
    cout<<"macos"<<endl;
#endif

    //for unfound exe, look in path

    for (QString exe : executables.keys())
    {
        if (executables[exe].isEmpty()) //if exe has not been found
        {
            for (QString p : path_split)
            {
                QString p_exe = QDir::cleanPath(p + QString("/") + exe);
                QFileInfo check_p_exe(p_exe);
                if (check_p_exe.exists() && check_p_exe.isExecutable())
                {
                    executables[exe]=p_exe;
                    break;
                }
            }
        }
    }

    QString unfoundExecMessage = QString ("Following executables unfound : ");
    bool atLeastOneExeMissing;
    for (QString exe : executables.keys())
    {
        if (executables[exe].isEmpty()) //if exe has not been found
        {
            unfoundExecMessage=unfoundExecMessage+QString("<b>")+exe+QString("</b>, ");
            atLeastOneExeMissing=true;
        }
    }
    if (atLeastOneExeMissing)
    {
        unfoundExecMessage.resize(unfoundExecMessage.size()-2);
        unfoundExecMessage+=QString(". Toggle <b>\"Show executables\"</b> in <b>Window</b> menu to find the missing executable path.");
        infoMsgBox(unfoundExecMessage,QMessageBox::Warning);
    }
}

void CSFWindow::readDefaultConfig()
{
    QString config_qstr;
    QFile config_qfile;
    config_qfile.setFileName(QString(":/config/default_config.json"));
    config_qfile.open(QIODevice::ReadOnly | QIODevice::Text);
    config_qstr = config_qfile.readAll();
    config_qfile.close();

    QJsonDocument config_doc = QJsonDocument::fromJson(config_qstr.toUtf8());
    QJsonObject root_obj = config_doc.object();

    if (m_data_found)
    {
        QJsonObject data_obj = root_obj["data"].toObject();
        lineEdit_BrainMask->setText(QDir::cleanPath(m_data_dir_path+QString("/masks/")+data_obj["BrainMask"].toString()));
        lineEdit_CerebellumMask->setText(QDir::cleanPath(m_data_dir_path+QString("/masks/")+data_obj["CerebellumMask"].toString()));
        lineEdit_T1img->setText(QDir::cleanPath(m_data_dir_path+QString("/inputs/")+data_obj["T1img"].toString()));
        lineEdit_T2img->setText(QDir::cleanPath(m_data_dir_path+QString("/inputs/")+data_obj["T2img"].toString()));
        lineEdit_TissueSeg->setText(QDir::cleanPath(m_data_dir_path+QString("/masks/")+data_obj["TissueSeg"].toString()));
        lineEdit_VentricleMask->setText(QDir::cleanPath(m_data_dir_path+QString("/masks/")+data_obj["VentricleMask"].toString()));
        lineEdit_OutputDir->setText(QDir::cleanPath(QDir::currentPath()+QString("/")+data_obj["output_dir"].toString()));
    }

    QJsonObject param_obj = root_obj["parameters"].toObject();
    spinBox_Index->setValue(param_obj["ACPC_index"].toInt());
    doubleSpinBox_mm->setValue(param_obj["ACPC_mm"].toDouble());
    spinBox_T1Weight->setValue(param_obj["ANTS_T1_weight"].toInt());
    doubleSpinBox_GaussianSigma->setValue(param_obj["ANTS_gaussian_sig"].toDouble());
    lineEdit_Iterations->setText(param_obj["ANTS_iterations_val"].toString());
    comboBox_RegType->setCurrentText(param_obj["ANTS_reg_type"].toString());
    comboBox_Metric->setCurrentText(param_obj["ANTS_sim_metric"].toString());
    spinBox_SimilarityParameter->setValue(param_obj["ANTS_sim_param"].toInt());
    doubleSpinBox_TransformationStep->setValue(param_obj["ANTS_transformation_step"].toDouble());
    lineEdit_ROIAtlasT1->setText(QDir::cleanPath(m_data_dir_path+QString("/atlases/")+param_obj["ROI_Atlas_T1"].toString()));
    lineEdit_ReferenceAtlasFile->setText(QDir::cleanPath(m_data_dir_path+QString("/atlases/")+param_obj["Reference_Atlas"].toString()));
    lineEdit_SSAtlasFolder->setText(QDir::cleanPath(m_data_dir_path+QString("/atlases/")+param_obj["SkullStripping_Atlases_dir"].toString()));
    lineEdit_TissueSegAtlas->setText(QDir::cleanPath(m_data_dir_path+QString("/atlases/")+param_obj["TissueSeg_Atlas_dir"].toString()));
    spinBox_CSFLabel->setValue(param_obj["TissueSeg_csf"].toInt());
    displayAtlases(lineEdit_SSAtlasFolder->text());

    QJsonArray exe_array = root_obj["executables"].toArray();
    for (QJsonValue exe_val : exe_array)
    {
        QJsonObject exe_obj = exe_val.toObject();
        executables[exe_obj["name"].toString()] = exe_obj["path"].toString();
    }

    QJsonArray script_array = root_obj["scripts"].toArray();
    for (QJsonValue script_val : script_array)
    {
        QJsonObject script_obj = script_val.toObject();
        for (QJsonValue exe_name : script_obj["executables"].toArray())
        {
            script_exe[script_obj["name"].toString()].append(exe_name.toString());
        }
    }
}

void CSFWindow::readConfig(QString filename)
{
    QString config_qstr;
    QFile config_qfile;
    config_qfile.setFileName(filename);
    config_qfile.open(QIODevice::ReadOnly | QIODevice::Text);
    config_qstr = config_qfile.readAll();
    config_qfile.close();

    QJsonDocument config_doc = QJsonDocument::fromJson(config_qstr.toUtf8());
    QJsonObject root_obj = config_doc.object();

    QJsonObject data_obj = root_obj["data"].toObject();
    lineEdit_BrainMask->setText(data_obj["BrainMask"].toString());
    lineEdit_CerebellumMask->setText(data_obj["CerebellumMask"].toString());
    lineEdit_T1img->setText(data_obj["T1img"].toString());
    lineEdit_T2img->setText(data_obj["T2img"].toString());
    lineEdit_TissueSeg->setText(data_obj["TissueSeg"].toString());
    lineEdit_VentricleMask->setText(data_obj["VentricleMask"].toString());
    lineEdit_OutputDir->setText(data_obj["output_dir"].toString());

    QJsonObject param_obj = root_obj["parameters"].toObject();
    spinBox_Index->setValue(param_obj["ACPC_index"].toInt());
    doubleSpinBox_mm->setValue(param_obj["ACPC_mm"].toDouble());
    spinBox_T1Weight->setValue(param_obj["ANTS_T1_weight"].toInt());
    doubleSpinBox_GaussianSigma->setValue(param_obj["ANTS_gaussian_sig"].toDouble());
    lineEdit_Iterations->setText(param_obj["ANTS_iterations_val"].toString());
    comboBox_RegType->setCurrentText(param_obj["ANTS_reg_type"].toString());
    comboBox_Metric->setCurrentText(param_obj["ANTS_sim_metric"].toString());
    spinBox_SimilarityParameter->setValue(param_obj["ANTS_sim_param"].toInt());
    doubleSpinBox_TransformationStep->setValue(param_obj["ANTS_transformation_step"].toDouble());
    lineEdit_ROIAtlasT1->setText(param_obj["ROI_Atlas_T1"].toString());
    lineEdit_ReferenceAtlasFile->setText(param_obj["Reference_Atlas"].toString());
    lineEdit_SSAtlasFolder->setText(param_obj["SkullStripping_Atlases_dir"].toString());
    lineEdit_TissueSegAtlas->setText(param_obj["TissueSeg_Atlas_dir"].toString());
    spinBox_CSFLabel->setValue(param_obj["TissueSeg_csf"].toInt());
    displayAtlases(lineEdit_SSAtlasFolder->text());

    QJsonArray exe_array = root_obj["executables"].toArray();
    for (QJsonValue exe_val : exe_array)
    {
        QJsonObject exe_obj = exe_val.toObject();
        executables[exe_obj["name"].toString()] = exe_obj["path"].toString();
    }
}

bool CSFWindow::writeConfig(QString filename)
{
    QFile saveFile(filename);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        cout<<"Couldn't open save file."<<endl;
        return false;
    }

    QJsonObject root_obj;
    QJsonObject data_obj;
    data_obj["BrainMask"] = lineEdit_BrainMask->text();
    data_obj["T1img"] = lineEdit_T1img->text();
    data_obj["T2img"] = lineEdit_T2img->text();
    data_obj["CerebellumMask"] = lineEdit_CerebellumMask->text();
    data_obj["VentricleMask"] = lineEdit_VentricleMask->text();
    data_obj["TissueSeg"] = lineEdit_TissueSeg->text();
    data_obj["output_dir"] = lineEdit_OutputDir->text();
    root_obj["data"]=data_obj;

    QJsonObject param_obj;
    param_obj["ACPC_index"] = spinBox_Index->value();
    param_obj["ACPC_mm"] = doubleSpinBox_mm->value();
    param_obj["ANTS_T1_weight"] = spinBox_T1Weight->value();
    param_obj["ANTS_gaussian_sig"] = doubleSpinBox_GaussianSigma->value();
    param_obj["ANTS_iterations_val"] = lineEdit_Iterations->text();
    param_obj["ANTS_reg_type"] = comboBox_RegType->currentText();
    param_obj["ANTS_sim_metric"] = comboBox_Metric->currentText();
    param_obj["ANTS_sim_param"] = spinBox_SimilarityParameter->value();
    param_obj["ANTS_transformation_step"] = doubleSpinBox_TransformationStep->value();
    param_obj["ROI_Atlas_T1"] = lineEdit_ROIAtlasT1->text();
    param_obj["Reference_Atlas"] = lineEdit_ReferenceAtlasFile->text();
    param_obj["SkullStripping_Atlases_dir"] = lineEdit_SSAtlasFolder->text();
    param_obj["TissueSeg_Atlas_dir"] = lineEdit_TissueSegAtlas->text();
    param_obj["TissueSeg_csf"] = spinBox_CSFLabel->value();
    root_obj["parameters"]=param_obj;

    QJsonArray exe_array;
    for (QString exe_name : executables.keys())
    {
        QJsonObject exe_obj;
        exe_obj["name"]=exe_name;
        exe_obj["path"]=executables[exe_name];
        exe_array.append(exe_obj);
    }
    root_obj["executables"]=exe_array;

    QJsonDocument saveDoc(root_obj);
    saveFile.write(saveDoc.toJson());

    cout<<"Saved configuration : "<<filename.toStdString()<<endl;
    return true;
}

QString CSFWindow::OpenFile(){
    QString filename = QFileDialog::getOpenFileName(
                this,
                tr("Open File"),
                "C://",
                "All files (*.*);; NIfTI File (*.nii *.nii.gz);; NRRD File  (*.nrrd);; Json File (*.json)"
                );
    return filename;
}

QString CSFWindow::OpenDir(){
    QString dirname = QFileDialog::getExistingDirectory(
                this,
                tr("Open Directory"),
                "C://",
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
                );
    return dirname;
}

QString CSFWindow::SaveFile()
  {
    QString filename = QFileDialog::getSaveFileName(
        this,
        tr("Save Document"),
        QDir::currentPath(),
        tr("JSON file (*.json)") );
    return filename;
  }

bool CSFWindow::lineEdit_isEmpty(QLineEdit*& le)
{
    if (le->text().isEmpty()){return true;}
    else
    {
        for (QChar c : le->text())
        {
            if (c != ' ')
            {
                return false;
            }
        }
        return true;
    }
}

bool CSFWindow::checkOptionalMasks()
{
    if (lineEdit_isEmpty(lineEdit_BrainMask)
            && lineEdit_isEmpty(lineEdit_VentricleMask)
            && lineEdit_isEmpty(lineEdit_TissueSeg)
            && lineEdit_isEmpty(lineEdit_CerebellumMask))

    {
        return false;
    }
    return true;
}

void CSFWindow::setBestDataAlignmentOption()
{
    radioButton_rigidRegistration->setChecked(!checkOptionalMasks());
    radioButton_rigidRegistration->setEnabled(!checkOptionalMasks());
    pushButton_ReferenceAtlasFile->setEnabled(!checkOptionalMasks());
    lineEdit_ReferenceAtlasFile->setEnabled(!checkOptionalMasks());
    label_dataAlignmentMessage->setVisible(checkOptionalMasks());
    radioButton_preAligned->setChecked(checkOptionalMasks());
}

int CSFWindow::questionMsgBox(bool checkState, QString maskType, QString action)
{
    QString selection;
    QString verb;
    QString qm;
    if (checkState)
    {
        selection=QString("have");
        verb=QString("Do ");
        qm=QString(" anyway?");
    }
    else
    {
        selection=QString("haven't");
        verb=QString("Don't");
        qm=QString("?");
    }

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowOpacity(1.0);
    msgBox.setText(QString("It seems you "+selection+" selected a "+maskType+" file."));
    msgBox.setInformativeText(QString(verb+" you want to proceed with automatic "+action+qm));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (checkState)
    {
        msgBox.setDefaultButton(QMessageBox::No);
    }
    else
    {
        msgBox.setDefaultButton(QMessageBox::Yes);
    }
    return msgBox.exec();
}

void CSFWindow::infoMsgBox(QString message, QMessageBox::Icon type)
{
    QMessageBox mb;
    mb.setIcon(type);
    mb.setText(message);
    mb.setStandardButtons(QMessageBox::Ok);
    mb.exec();
}

void CSFWindow::displayAtlases(QString folder_path)
{
    listWidget_SSAtlases->clear();
    const QString T1=QString("T1");
    const QString T2=QString("T2");
    const QString bmask=QString("brainmask");
    QDir folder(folder_path);
    QStringList itemsList=QStringList(QString("Select all"));
    QStringList invalidItems=QStringList();
    QMap<QString,QStringList> atlases=QMap<QString,QStringList>();
    QStringList splittedName=QStringList();
    QString atlasName=QString();
    QString fileType=QString();
    QStringList fileSplit=QStringList();
    QString fileNameBase=QString();

    for (QString fileName : folder.entryList())
    {
        fileSplit=fileName.split('.');
        fileNameBase=fileSplit.at(0);

        if (fileSplit.size()>3 || fileSplit.size()<2)
        {
            invalidItems.append(fileName);
        }
        else
        {
            QString ext=QString();
            QString ext_ref=QString();
            if (fileSplit.size()==3)
            {
                ext=fileSplit.at(1)+fileSplit.at(2);
                ext_ref=QString("niigz");
            }
            else
            {
                ext=fileSplit.at(1);
                ext_ref=QString("nrrd");
            }

            if (ext == ext_ref)
            {
                splittedName=fileNameBase.split('_');
                fileType=splittedName.at(splittedName.size()-1);
                splittedName.pop_back();
                atlasName=QString(splittedName.join('_'));


                if(fileType == bmask)
                {
                    atlases.insert(atlasName,{bmask});
                }
                else if((fileType == T1) && atlases.contains(atlasName))
                {
                    atlases[atlasName].append(T1);
                }
                else if((fileType == T2) && atlases.contains(atlasName))
                {
                    atlases[atlasName].append(T2);
                }
            }
            else if((fileName != QString(".")) && (fileName != QString("..")))
            {
                invalidItems.append(fileName);
            }
        }


    }

    for (QString atlas : atlases.keys())
    {
        QString itemLabel=QString();
        if (atlases[atlas].size() == 2)
        {
            itemLabel = atlas+QString(" : ")+atlases[atlas].at(0)+QString(" and ")+atlases[atlas].at(1)+QString(" image detected");
        }
        else if(atlases[atlas].size() == 3)
        {
            itemLabel = atlas+QString(" : ")+atlases[atlas].at(0)+QString(", ")+atlases[atlas].at(1)+QString(" and ")+atlases[atlas].at(2)+QString(" images detected");
        }
        itemsList.append(itemLabel);
    }

    listWidget_SSAtlases->addItems(itemsList);


    QListWidgetItem *item=0;
    for (int i = 0 ; i < listWidget_SSAtlases->count(); i++)
    {
        item = listWidget_SSAtlases->item(i);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        if (i==0)
        {
            QFont b_font=QFont();
            b_font.setBold(true);
            item->setFont(b_font);
        }
    }
    int count=listWidget_SSAtlases->count();

    listWidget_SSAtlases->addItems(invalidItems);
    for (int i = count ; i < listWidget_SSAtlases->count(); i++)
    {
        item = listWidget_SSAtlases->item(i);
        item->setTextColor(QColor(150,150,150));
    }

    QObject::connect(listWidget_SSAtlases, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(selectAtlases(QListWidgetItem*)));
}

void CSFWindow::write_main_script()
{
    QString CSFLabel=QString::number(spinBox_CSFLabel->value());
    QString CerebMask=lineEdit_CerebellumMask->text();
    QString BrainMask=lineEdit_BrainMask->text();

    QFile file(QString(":/PythonScripts/main_script.py"));
    file.open(QIODevice::ReadOnly);
    QString script = file.readAll();
    file.close();

    script.replace("@T1IMG@", T1img);
    if (lineEdit_isEmpty(lineEdit_T2img))
    {
        script.replace("@T2IMG@", QString(""));
    }
    else
    {
        script.replace("@T2IMG@", T2img);
    }
    script.replace("@BRAIN_MASK@", BrainMask);
    script.replace("@CEREB_MASK@", CerebMask);
    script.replace("@TISSUE_SEG@", TissueSeg);
    script.replace("@CSF_LABEL@",CSFLabel);
    if(radioButton_Index->isChecked())
    {
        script.replace("@ACPC_UNIT@", QString("index"));
        script.replace("@ACPC_VAL@", QString::number(spinBox_Index->value()));
    }
    else
    {
        script.replace("@ACPC_UNIT@", QString("mm"));
        script.replace("@ACPC_VAL@", QString::number(doubleSpinBox_mm->value()));
    }

    script.replace("@USE_DCM@", QString(lineEdit_isEmpty(lineEdit_CerebellumMask) ? "true" : "false"));
    script.replace("@PERFORM_REG@", QString(radioButton_rigidRegistration->isChecked() ? "true" : "false"));
    script.replace("@PERFORM_SS@", QString(checkBox_SkullStripping->isChecked() ? "true" : "false"));
    script.replace("@PERFORM_TSEG@", QString(checkBox_TissueSeg->isChecked() ? "true" : "false"));
    script.replace("@PERFORM_VR@", QString(checkBox_VentricleRemoval->isChecked() ? "true" : "false"));

    for (QString exe_name : script_exe[QString("main_script")])
    {
        QString str_code = QString("@")+exe_name+QString("_PATH@");
        QString exe_path = executables[exe_name];
        script.replace(str_code,exe_path);
    }

    script.replace("@DATADIR@", m_data_dir_path);
    script.replace("@OUTPUT_DIR@", output_dir);

    QString main_script = QDir::cleanPath(scripts_dir + QString("/main_script.py"));
    QFile outfile(main_script);
    outfile.open(QIODevice::WriteOnly);
    QTextStream outstream(&outfile);
    outstream << script;
    outfile.close();
}

void CSFWindow::write_rigid_align()
{
    QString RefAtlasFile=lineEdit_ReferenceAtlasFile->text();

    QFile rgd_file(QString(":/PythonScripts/rigid_align.py"));
    rgd_file.open(QIODevice::ReadOnly);
    QString rgd_script = rgd_file.readAll();
    rgd_file.close();

    rgd_script.replace("@T1IMG@", T1img);
    if (lineEdit_isEmpty(lineEdit_T2img))
    {
        rgd_script.replace(QString("<IMAGE><FILE>@T2IMG@</FILE><ORIENTATION>file</ORIENTATION></IMAGE>"), QString(""));
    }
    else
    {
        rgd_script.replace(QString("@T2IMG@"), T2img);
    }
    rgd_script.replace("@ATLAS@", RefAtlasFile);

    for (QString exe_name : script_exe[QString("rigid_align")])
    {
        QString str_code = QString("@")+exe_name+QString("_PATH@");
        QString exe_path = executables[exe_name];
        rgd_script.replace(str_code,exe_path);
        //cout<<"exe name : "<<exe_name.toStdString()<<" ; "<<str_code.toStdString()<<" ; "<<exe_path.toStdString()<<endl;
    }

    rgd_script.replace("@OUTPUT_DIR@", output_dir);

    QString rigid_align_script = QDir::cleanPath(scripts_dir + QString("/rigid_align_script.py"));
    QFile rgd_outfile(rigid_align_script);
    rgd_outfile.open(QIODevice::WriteOnly);
    QTextStream rgd_outstream(&rgd_outfile);
    rgd_outstream << rgd_script;
    rgd_outfile.close();
}

void CSFWindow::write_make_mask()
{
    QString SSAtlases_dir=lineEdit_SSAtlasFolder->text();
    QStringList SSAtlases_list;
    QString SSAtlases_selected;
    QListWidgetItem *item=0;
    for (int i=0; i<listWidget_SSAtlases->count(); i++)
    {
        item=listWidget_SSAtlases->item(i);
        if (item->textColor() != QColor(150,150,150) && i!=0)
        {
            if (item->checkState() == Qt::Checked)
            {
                QStringList itemSplit = item->text().split(' ');
                SSAtlases_list.append(itemSplit[0]);
                if (itemSplit[4] == QString("T1"))
                {
                    SSAtlases_list.append(QString("1"));
                }
                else if (itemSplit[4] == QString("T2"))
                {
                    SSAtlases_list.append(QString("2"));
                }
                else
                {
                    SSAtlases_list.append(QString("3"));
                }
            }
        }
    }
    SSAtlases_selected=SSAtlases_list.join(',');

    QFile msk_file(QString(":/PythonScripts/make_mask.py"));
    msk_file.open(QIODevice::ReadOnly);
    QString msk_script = msk_file.readAll();
    msk_file.close();

    msk_script.replace("@T1IMG@", T1img);
    if (lineEdit_isEmpty(lineEdit_T2img))
    {
        msk_script.replace("@T2IMG@", QString(""));
    }
    else
    {
        msk_script.replace("@T2IMG@", T2img);
    }
    msk_script.replace("@ATLASES_DIR@", SSAtlases_dir);
    msk_script.replace("@ATLASES_LIST@", SSAtlases_selected);

    for (QString exe_name : script_exe[QString("make_mask")])
    {
        QString str_code = QString("@")+exe_name+QString("_PATH@");
        QString exe_path = executables[exe_name];
        msk_script.replace(str_code,exe_path);
        //cout<<"exe name : "<<exe_name.toStdString()<<" ; "<<str_code.toStdString()<<" ; "<<exe_path.toStdString()<<endl;
    }

    msk_script.replace("@OUTPUT_DIR@", output_dir);

    QString make_mask_script = QDir::cleanPath(scripts_dir + QString("/make_mask_script.py"));
    QFile msk_outfile(make_mask_script);
    msk_outfile.open(QIODevice::WriteOnly);
    QTextStream msk_outstream(&msk_outfile);
    msk_outstream << msk_script;
    msk_outfile.close();
}

void CSFWindow::write_tissue_seg()
{
    QString TS_Atlas_dir=lineEdit_TissueSegAtlas->text();
    QFile seg_file(QString(":/PythonScripts/tissue_seg.py"));
    seg_file.open(QIODevice::ReadOnly);
    QString seg_script = seg_file.readAll();
    seg_file.close();

    seg_script.replace("@T1IMG@", T1img);
    seg_script.replace("@T2IMG@", T2img);
    seg_script.replace("@ATLASES_DIR@", TS_Atlas_dir);

    for (QString exe_name : script_exe[QString("tissue_seg")])
    {
        QString str_code = QString("@")+exe_name+QString("_PATH@");
        QString exe_path = executables[exe_name];
        seg_script.replace(str_code,exe_path);
        //cout<<"exe name : "<<exe_name.toStdString()<<" ; "<<str_code.toStdString()<<" ; "<<exe_path.toStdString()<<endl;
    }

    seg_script.replace("@OUTPUT_DIR@", output_dir);

    QString tissue_seg_script = QDir::cleanPath(scripts_dir + QString("/tissue_seg_script.py"));
    QFile seg_outfile(tissue_seg_script);
    seg_outfile.open(QIODevice::WriteOnly);
    QTextStream seg_outstream(&seg_outfile);
    seg_outstream << seg_script;
    seg_outfile.close();
}

void CSFWindow::write_ABCxmlfile(bool T2provided)
{
    QFile xmlFile(QDir::cleanPath(output_dir+QString("/ABCparam.xml")));
    xmlFile.open(QIODevice::WriteOnly);
    QTextStream xmlStream(&xmlFile);
    xmlStream << "<?xml version=\"1.0\"?>" << '\n' << "<!DOCTYPE SEGMENTATION-PARAMETERS>" << '\n' << "<SEGMENTATION-PARAMETERS>" << '\n';
    xmlStream << "<SUFFIX>EMS</SUFFIX>" << '\n' << "<ATLAS-DIRECTORY>" << QDir::cleanPath(output_dir+QString("/TissueSegAtlas")) << "</ATLAS-DIRECTORY>" << '\n' << "<ATLAS-ORIENTATION>file</ATLAS-ORIENTATION>" <<'\n';
    xmlStream << "<OUTPUT-DIRECTORY>" << QDir::cleanPath(output_dir+QString("/ABC_Segmentation")) << "</OUTPUT-DIRECTORY>" << '\n' << "<OUTPUT-FORMAT>Nrrd</OUTPUT-FORMAT>" << '\n';
    xmlStream << "<IMAGE>" << '\n' << "\t<FILE>" << T1img << "</FILE>" << '\n' << "\t<ORIENTATION>file</ORIENTATION>" << '\n' << "</IMAGE>" << '\n';
    if (T2provided)
    {
        xmlStream << "<IMAGE>" << '\n' << "\t<FILE>" << T2img << "</FILE>" << '\n' << "\t<ORIENTATION>file</ORIENTATION>" << '\n' << "</IMAGE>" << '\n';
    }
    xmlStream << "<FILTER-ITERATIONS>10</FILTER-ITERATIONS>" << '\n' << "<FILTER-TIME-STEP>0.01</FILTER-TIME-STEP>" << '\n' << "<FILTER-METHOD>Curvature flow</FILTER-METHOD>" << '\n';
    xmlStream << "<MAX-BIAS-DEGREE>4</MAX-BIAS-DEGREE>" << '\n' << "<PRIOR>1.3</PRIOR>" << '\n' << "<PRIOR>1</PRIOR>" << '\n' << "<PRIOR>0.7</PRIOR>" << '\n';
    xmlStream << "<PRIOR>1</PRIOR>" << '\n' << "<PRIOR>0.8</PRIOR>" << '\n' << "<INITIAL-DISTRIBUTION-ESTIMATOR>robust</INITIAL-DISTRIBUTION-ESTIMATOR>" << '\n';
    xmlStream << "<DO-ATLAS-WARP>0</DO-ATLAS-WARP>" << '\n' << "<ATLAS-WARP-FLUID-ITERATIONS>50</ATLAS-WARP-FLUID-ITERATIONS>" << '\n';
    xmlStream << "<ATLAS-WARP-FLUID-MAX-STEP>0.1</ATLAS-WARP-FLUID-MAX-STEP>" << '\n' << "<ATLAS-LINEAR-MAP-TYPE>id</ATLAS-LINEAR-MAP-TYPE>" << '\n';
    xmlStream << "<IMAGE-LINEAR-MAP-TYPE>id</IMAGE-LINEAR-MAP-TYPE>" << '\n' << "</SEGMENTATION-PARAMETERS>" << endl;
    xmlFile.close();
}

void CSFWindow::write_vent_mask()
{
    QString ROI_AtlasT1=lineEdit_ROIAtlasT1->text();
    QString VentricleMask=lineEdit_VentricleMask->text();

    QFile v_file(QString(":/PythonScripts/vent_mask.py"));
    v_file.open(QIODevice::ReadOnly);
    QString v_script = v_file.readAll();
    v_file.close();

    v_script.replace("@T1IMG@", T1img);
    v_script.replace("@ATLAS@", ROI_AtlasT1);
    v_script.replace("@REG_TYPE@", Registration_Type);
    v_script.replace("@TRANS_STEP@", Transformation_Step);
    v_script.replace("@ITERATIONS@", Iterations);
    v_script.replace("@SIM_METRIC@", Sim_Metric);
    v_script.replace("@SIM_PARAMETER@", Sim_Parameter);
    v_script.replace("@GAUSSIAN@", Gaussian);
    v_script.replace("@T1_WEIGHT@", T1_Weight);
    v_script.replace("@TISSUE_SEG@", TissueSeg);
    v_script.replace("@VENTRICLE_MASK@" ,VentricleMask);

    for (QString exe_name : script_exe[QString("vent_mask")])
    {
        QString str_code = QString("@")+exe_name+QString("_PATH@");
        QString exe_path = executables[exe_name];
        v_script.replace(str_code,exe_path);
        //cout<<"exe name : "<<exe_name.toStdString()<<" ; "<<str_code.toStdString()<<" ; "<<exe_path.toStdString()<<endl;
    }

    v_script.replace("@OUTPUT_DIR@", output_dir);
    v_script.replace("@OUTPUT_MASK@", "_AtlasToVent.nrrd");

    QString vent_mask_script = QDir::cleanPath(scripts_dir + QString("/vent_mask_script.py"));
    QFile v_outfile(vent_mask_script);
    v_outfile.open(QIODevice::WriteOnly);
    QTextStream v_outstream(&v_outfile);
    v_outstream << v_script;
    v_outfile.close();
}

//SLOTS
// Logs
void CSFWindow::disp_output()
{
    QString output(prc->readAllStandardOutput());
    out_log->append(output);
}

void CSFWindow::disp_err()
{
    QString errors(prc->readAllStandardError());
    errors="<font color=\"red\"><b>While running : </b></font>"+executables["python3"]+"<br><font color=\"red\"><b>Following error(s) occured : </b></font><br>"+errors;
    for (QString sc_name : script_exe.keys())
    {
        if (sc_name!="main_script")
        {
            sc_name=sc_name+"_script";
        }
        sc_name=sc_name+".py";
        errors.replace(sc_name,"<b>"+sc_name+"</b>");
    }
    err_log->append(errors);
}

void CSFWindow::prc_finished(int exitCode, QProcess::ExitStatus exitStatus){
    QString exit_message;
    exit_message = QString("Auto_EACSF pipeline ") + ((exitStatus == QProcess::NormalExit) ? QString("exited with code ") + QString::number(exitCode) : QString("crashed"));
    if (exitCode==0)
    {
        exit_message="<font color=\"green\"><b>"+exit_message+"</b></font>";
    }
    else
    {
        exit_message="<font color=\"red\"><b>"+exit_message+"</b></font>";
    }
    out_log->append(exit_message);
    cout<<exit_message.toStdString()<<endl;
}

//File menu
void CSFWindow::on_actionLoad_Configuration_File_triggered()
{
    QString filename= OpenFile();
    if (!filename.isEmpty())
    {
        readConfig(filename);
    }
}

void CSFWindow::on_actionSave_Configuration_triggered(){
    QString filename=SaveFile();
    if (!filename.isEmpty())
    {
        if (!filename.endsWith(QString(".json")))
        {
            filename+=QString(".json");
        }
        bool success=writeConfig(filename);
        if (success)
        {
            infoMsgBox(QString("Configuration saved with success : ")+filename,QMessageBox::Information);
        }
        else
        {
            infoMsgBox(QString("Couldn't save configuration file at this location. Try somewhere else."),QMessageBox::Warning);
        }
    }

}

//Window menu
void CSFWindow::on_actionShow_executables_toggled(bool toggled)
{
    if (toggled)
    {
        tabWidget->insertTab(1,tab_executables,QString("Executables"));
    }
    else
    {
        tabWidget->removeTab(1);
    }

}

//About
void CSFWindow::on_actionAbout_triggered()
{
    QString messageBoxTitle = "About " + QString( Auto_EACSF_TITLE );
        QString aboutFADTTS;
        aboutFADTTS = "<b>Version:</b> " + QString( Auto_EACSF_VERSION ) + "<br>"
                "<b>Contributor(s):</b> " + QString( Auto_EACSF_CONTRIBUTORS ) + "<br>"
                "<b>License:</b> Apache 2.0<br>" +
                "<b>Github:</b> <a href=" + m_github_url + ">Click here</a><br>";
    QMessageBox::information( this, tr( qPrintable( messageBoxTitle ) ), tr( qPrintable( aboutFADTTS ) ), QMessageBox::Ok );
}

// 1st Tab - Inputs
void CSFWindow::on_pushButton_T1img_clicked()
{
    QString path=OpenFile();
    if (!path.isEmpty())
    {
        lineEdit_T1img->setText(path);
    }
}

void CSFWindow::on_pushButton_T2img_clicked()
{
    QString path=OpenFile();
    if (!path.isEmpty())
    {
        lineEdit_T2img->setText(path);
    }
}

void CSFWindow::on_pushButton_BrainMask_clicked()
{
    QString path=OpenFile();
    if (!path.isEmpty())
    {
        lineEdit_BrainMask->setText(path);
    }
}

void CSFWindow::on_lineEdit_BrainMask_textChanged()
{
    setBestDataAlignmentOption();
    checkBox_SkullStripping->setChecked(lineEdit_isEmpty(lineEdit_BrainMask));
}

void CSFWindow::on_pushButton_VentricleMask_clicked()
{
    QString path=OpenFile();
    if (!path.isEmpty())
    {
        lineEdit_VentricleMask->setText(path);
    }
}

void CSFWindow::on_pushButton_TissueSeg_clicked()
{
    QString path=OpenFile();
    if (!path.isEmpty())
    {
        lineEdit_TissueSeg->setText(path);
    }
}

void CSFWindow::on_lineEdit_TissueSeg_textChanged()
{
    setBestDataAlignmentOption();
    checkBox_TissueSeg->setChecked(lineEdit_isEmpty(lineEdit_TissueSeg));
}

void CSFWindow::on_CerebellumMask_clicked()
{
    QString path=OpenFile();
    if (!path.isEmpty())
    {
        lineEdit_CerebellumMask->setText(path);
    }
}

void CSFWindow::on_lineEdit_CerebellumMask_textChanged()
{
    setBestDataAlignmentOption();
}

void CSFWindow::on_pushButton_OutputDir_clicked()
{
    QString path=OpenDir();
    if (!path.isEmpty())
    {
        lineEdit_OutputDir->setText(path);
    }
}

void CSFWindow::on_radioButton_Index_clicked(const bool checkState)
{
    doubleSpinBox_mm->setEnabled(!checkState);
    spinBox_Index->setEnabled(checkState);
}

void CSFWindow::on_radioButton_mm_clicked(const bool checkState)
{
    doubleSpinBox_mm->setEnabled(checkState);
    spinBox_Index->setEnabled(!checkState);
}

// Executables tab
void CSFWindow::updateExecutables(QString exeName, QString path)
{
    executables[exeName]=path;
}

// 3rd Tab - 1.Reference Alignment, 2.Skull Stripping

void CSFWindow::on_pushButton_ReferenceAtlasFile_clicked()
{
    QString path=OpenFile();
    if (!path.isEmpty())
    {
        lineEdit_ReferenceAtlasFile->setText(path);
    }
}


void CSFWindow::on_checkBox_SkullStripping_clicked(const bool checkState)
{
    if (checkState != lineEdit_isEmpty(lineEdit_BrainMask))
    {
        int ret=questionMsgBox(checkState,QString("brain mask"),QString("skull stripping"));
        if ((checkState && ret==QMessageBox::No) || (!checkState && ret==QMessageBox::Yes))
        {
            checkBox_SkullStripping->setChecked(!checkState);
        }
    }
}

void CSFWindow::on_checkBox_SkullStripping_stateChanged(int state)
{
    bool enab;
    if (state==Qt::Checked){enab=true;}
    else{enab=false;}

    lineEdit_SSAtlasFolder->setEnabled(enab);
    pushButton_SSAtlasFolder->setEnabled(enab);
    listWidget_SSAtlases->setEnabled(enab);
}

void CSFWindow::on_pushButton_SSAtlasFolder_clicked()
{
    QString path=OpenDir();
    if (!path.isEmpty())
    {
    lineEdit_SSAtlasFolder->setText(path);
    }
    displayAtlases(lineEdit_SSAtlasFolder->text());
}

void CSFWindow::selectAtlases(QListWidgetItem *item)
{
    if (item->text() == QString("Select all"))
    {
        QListWidgetItem *it=0;
        for (int i = 0; i < listWidget_SSAtlases->count(); i++)
        {
            it=listWidget_SSAtlases->item(i);
            if (it->textColor() != QColor(150,150,150))
            {
                it->setCheckState(item->checkState());
            }
        }
    }
}

// 4th Tab - 3.Tissue Seg
void CSFWindow::on_checkBox_TissueSeg_clicked(const bool checkState)
{
    if (checkState != lineEdit_isEmpty(lineEdit_TissueSeg))
    {
        int ret=questionMsgBox(checkState,QString("tissue segmentation"),QString("tissue segmentation"));
        if ((checkState && ret==QMessageBox::No) || (!checkState && ret==QMessageBox::Yes))
        {
            checkBox_TissueSeg->setChecked(!checkState);
        }
    }
}

void CSFWindow::on_checkBox_TissueSeg_stateChanged(int state)
{
    bool enab;
    if (state==Qt::Checked){enab=true;}
    else{enab=false;}

    label_TissueSeg->setEnabled(enab);
    pushButton_TissueSegAtlas->setEnabled(enab);
    lineEdit_TissueSegAtlas->setEnabled(enab);
}


void CSFWindow::on_pushButton_TissueSegAtlas_clicked()
{
    QString path=OpenDir();
    if (!path.isEmpty())
    {
        lineEdit_TissueSegAtlas->setText(path);
    }
}

// 5th Tab - 4.Ventricle Masking
void CSFWindow::on_checkBox_VentricleRemoval_stateChanged(int state)
{
    bool enab;
    if (state==Qt::Checked){enab=true;}
    else{enab=false;}

    label_VentricleRemoval->setEnabled(enab);
    pushButton_ROIAtlasT1->setEnabled(enab);
    lineEdit_ROIAtlasT1->setEnabled(enab);
}

void CSFWindow::on_pushButton_ROIAtlasT1_clicked()
{QString path=OpenFile();
    if (!path.isEmpty())
    {
        lineEdit_ROIAtlasT1->setText(path);
    }
}

// 6th Tab - ANTS Registration
void CSFWindow::on_comboBox_RegType_currentTextChanged(const QString &val)
{
    Registration_Type=val;
}

void CSFWindow::on_doubleSpinBox_TransformationStep_valueChanged()
{

}

void CSFWindow::on_comboBox_Metric_currentTextChanged(const QString &val)
{
    Sim_Metric=val;
}

void CSFWindow::on_lineEdit_Iterations_textChanged(const QString &val)
{
    Iterations=val;
}

void CSFWindow::on_spinBox_SimilarityParameter_valueChanged(const int val)
{
    Sim_Parameter=val;
}

void CSFWindow::on_doubleSpinBox_GaussianSigma_valueChanged(const double val)
{
    Gaussian=val;
}

void CSFWindow::on_spinBox_T1Weight_valueChanged(const QString &val)
{
    T1_Weight=val;
}

// 7th Tab - Execution
// Execute
void CSFWindow::on_pushButton_execute_clicked()
{
    //0. WRITE MAIN_SCRIPT

    //MAIN_KEY WORDS
    T1img=lineEdit_T1img->text();
    T2img=lineEdit_T2img->text();
    TissueSeg=lineEdit_TissueSeg->text();
    output_dir=lineEdit_OutputDir->text();
    scripts_dir=QDir::cleanPath(output_dir+QString("/PythonScripts"));

    QDir out_dir=QDir();
    out_dir.mkdir(output_dir);

    QDir sc_dir=QDir();
    sc_dir.mkdir(scripts_dir);

    //0. WRITE MAIN_SCRIPT
    write_main_script();

    //1. WRITE RIGID_ALIGN_SCRIPT
    write_rigid_align();

    //2. WRITE MAKE_MASK_SCRIPT
    write_make_mask();

    //3. WRITE TISSUE_SEG_SCRIPT
    write_tissue_seg();

    //4. WRITE Auto_SEG XML
    write_ABCxmlfile(!lineEdit_isEmpty(lineEdit_T2img));

    //5. WRITE VENT_MASK_SCRIPT
    write_vent_mask();

    //Notification
    QMessageBox::information(
        this,
        tr("Auto EACSF"),
        tr("Python scripts are running. It may take up to 24 hours to process.")
    );

    // RUN PYTHON    

    QString main_script = QDir::cleanPath(scripts_dir + QString("/main_script.py"));
    QStringList params = QStringList() << main_script;

    connect(prc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(prc_finished(int, QProcess::ExitStatus)));
    connect(prc, SIGNAL(started()), this, SLOT(prc_started()));
    connect(prc, SIGNAL(readyReadStandardOutput()), this, SLOT(disp_output()));
    connect(prc, SIGNAL(readyReadStandardError()), this, SLOT(disp_err()));
    prc->setWorkingDirectory(output_dir);
    prc->start(executables[QString("python3")], params);
}
