// Copyright 2017,2018,2019,2020,2021 Sony Corporation.
// Copyright 2021,2022,2023,2024,2025 Sony Group Corporation.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//---------------------------------------------------------------------------

#ifndef sdeep_console_main_formH
#define sdeep_console_main_formH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <FMX.Controls.hpp>
#include <FMX.Forms.hpp>
#include <FMX.StdCtrls.hpp>
#include <FMX.Types.hpp>
#include <FMX.Objects.hpp>
#include <FMX.TabControl.hpp>
#include <FMX.Layouts.hpp>
#include <FMX.TreeView.hpp>
#include <FMX.Grid.hpp>
#include <FMX.Memo.hpp>
#include <FMX.Dialogs.hpp>
#include <FMX.Edit.hpp>
#include <FMX.Menus.hpp>
#include <FMX.ListBox.hpp>
#include <FMX.Media.hpp>
//---------------------------------------------------------------------------
#include <IdBaseComponent.hpp>
#include <IdComponent.hpp>
#include <IdTCPClient.hpp>
#include <IdTCPConnection.hpp>
#include <FMX.Ani.hpp>
#include <FMX.ExtCtrls.hpp>
#include <IdIOHandler.hpp>
#include <IdIOHandlerSocket.hpp>
#include <IdIOHandlerStack.hpp>
#include <IdSSL.hpp>
#include <IdSSLOpenSSL.hpp>
#include <FMX.Controls.Presentation.hpp>
#include <FMX.ScrollBox.hpp>
#include <FMX.Controls3D.hpp>
#include <FMX.Layers3D.hpp>
#include <System.Math.Vectors.hpp>
#include <FMX.Grid.Style.hpp>
#include <System.Rtti.hpp>
//---------------------------------------------------------------------------
#include "nn_document.h"
#include "sdeep_console_global_settings.h"
#include "nn_graph_mapper.h"
#include "sdeep_training_thread.h"
#include "sdeep_evaluation_thread.h"
#include "imagecache.h"
#include "csvcache.h"
#include "list_frame.h"
#include "nncplugin.h"
#include <FMX.Memo.Types.hpp>
//---------------------------------------------------------------------------
using namespace elfe;
using namespace sdeep_console;
//---------------------------------------------------------------------------
enum class SDeepConsoleActivity : int {
Default,
Browser,
};
//---------------------------------------------------------------------------
enum class NetworkMouseDownMode : int {
None,
DragLayer,
DragLink,
RangeSelect,
};
//---------------------------------------------------------------------------
enum class ComponentDragMode : int {
None,
BeforeAddComponent,
AfterAddComponent,
};
//---------------------------------------------------------------------------
enum class ControlButtonType : int {
Run,
Stop,
RunTraining,
ScheduleTraining,
StopAll,
StopTraining,
RunEvaluation,
StopEvaluation,
RunDisabled,
RunDisabledWarning,//Display warning,
VisibleFalse
};
//---------------------------------------------------------------------------
enum class DocumentIOType : int {
Open,
Save,
OpenDataset,
CreateDataset,
OpenResultCSV,
GenerateCCode,
ExtractWeight_NNabla,
ExtractNnpt,
PythonCall,
Auth,
UploadDataset,
Import,
Export,
ExportReport,
};

typedef struct table_ui_state {
	double left, top;
	double current_left, current_top;
	int col_index, row_index;
	int cols_in_screen, rows_in_screen;
	TRectD cursor_rect, current_cursor_rect;
} TableUIState;
//---------------------------------------------------------------------------
enum class ConfigType : int {
Global,
Optimizer,
Monitor,
Executor,
};
//---------------------------------------------------------------------------
enum class OthersTableType : int {
SimpleTable,
Dataset,
Profile,
Default,
};
//---------------------------------------------------------------------------
class TDocumentIOFunc : public TCppInterfacedObject<TProc>
{
public:
	TDocumentIOFunc(NNDocument *document, SDeepConsoleGlobalSettings *settings, UnicodeString file_name, DocumentIOType type, int option, int debug) :
		document_(document), settings_(settings), file_name_(file_name), type_(type), option_(option), debug_(debug), result_(0) {}
	virtual void __fastcall Invoke();
	void __fastcall DocumentUnlock() { if (document_->GetLocked()) {document_->Unlock(); }};
	void __fastcall CallRefreshLayerComponent();

	void __fastcall SyncAndShowMessage(const char *msg) {message_ = msg; TThread::Synchronize(TThread::CurrentThread, ShowMsg);};
	void __fastcall ShowMsg() {ShowMessage(message_);};
	int __fastcall SyncAndIndicatorLog(const char *msg) {message_ = msg; TThread::Synchronize(TThread::CurrentThread, IndicatorLog); return result_;};
	void __fastcall IndicatorLog();

	void __fastcall Close();
private:
	NNDocument *document_;
	SDeepConsoleGlobalSettings *settings_;
	UnicodeString file_name_;
	DocumentIOType type_;
	int option_;
	int debug_;
	UnicodeString message_;

	int result_;
};
//---------------------------------------------------------------------------
class TFo_Main : public TForm
{
__published:
	TButton *Bu_Save;
	TTabControl *TC_Default;
	TTabItem *TI_Config;
	TTabItem *TI_Dataset;
	TTabItem *TI_Network;
	TLayout *Lo_NetworkClient;
	TStyleBook *StyleBook;
	TTabItem *TI_Debug;
	TMemo *Me_Debug;
	TButton *Bu_SaveAs;
	TImage *Im_SaveAs;
	TOpenDialog *OD_Project;
	TSaveDialog *SD_Project;
	TLayout *Lo_Property;
	TPaintBox *PB_Property;
	TLabel *La_LayerProperty;
	TButton *Bu_Property;
	TLabel *La_NetworkStatistics;
	TPaintBox *PB_Statistics;
	TButton *Bu_Statistics;
	TButton *Bu_Setup;
	TImage *Im_Setup;
	TOpenDialog *OD_CSV;
	TPaintBox *PB_Table;
	TButton *Bu_Table;
	TButton *Bu_Back;
	TFloatAnimation *FA_TCDefaultScaleX;
	TFloatAnimation *FA_TCDefaultScaleY;
	TFloatAnimation *FA_TCDefaultPositionX;
	TFloatAnimation *FA_TCDefaultPositionY;
	TSaveDialog *SD_XML;
	TOpenDialog *OD_XML;
	TTabItem *TI_Results;
	TSplitter *Sp_TrainingMonitorBottom;
	TPaintBox *PB_Monitor;
	TTabItem *TI_Evaluation;
	TComboBox *Co_Property;
	TPopupMenu *PM_Monitor;
	TMenuItem *MIM_OpenResultLocation;
	TScrollBar *Sc_TableX;
	TPopupMenu *PM_Table;
	TMenuItem *MIT_OpenData;
	TPopupMenu *PM_ListResults;
    TLayout *Lo_MonitorGain;
	TMenuItem *MILR_OpenResultLocation;
	TMenuItem *MILR_Separator3;
	TMenuItem *MILR_DeleteFromDisk;
	TMenuItem *MILR_DeleteAllIncompleteResultsFromDisk;
	TMenuItem *MILR_Separator2;
	TLabel *La_Test;
	TPanel *Pa_Processing;
	TAniIndicator *AI_Processing;
	TLabel *La_Processing;
	TMenuItem *MILR_OpenLearningCurveForComparison;
	TMenuItem *MILR_ClearLearningCurveForComparison;
	TSaveDialog *SD_Image;
	TLayout *Lo_Result;
	TPaintBox *PB_Network;
	TPopupMenu *PM_Network;
	TMenuItem *MIN_Undo;
	TMenuItem *MIN_Redo;
	TMenuItem *MIN_Sep0;
	TMenuItem *MIN_Cut;
	TMenuItem *MIN_Copy;
	TMenuItem *MIN_Paste;
	TMenuItem *MIN_Delete;
	TMenuItem *MIN_Sep1;
	TMenuItem *MIN_SelectAll;
	TMenuItem *MIN_InverseSelection;
	TMenuItem *MIN_ClearSelection;
	TMenuItem *MIN_Sep2;
	TMenuItem *MIN_ArrangeLayers;
	TMenuItem *MIN_Sep3;
	TMenuItem *MIN_SaveNetworkBitmapAs;
	TButton *Bu_Network;
	TScrollBar *Sc_NetworkY;
	TScrollBar *Sc_NetworkX;
	TMenuItem *MILR_Separator1;
	TCalloutPanel *Pa_Hint;
	TLabel *La_Hint;
	TFloatAnimation *FA_Hint;
	TButton *Bu_FullScreen;
	TImage *Im_FullScreen;
	TMenuItem *MIT_Separator;
	TMenuItem *MIT_CheckConsistency;
	TMenuItem *MIN_Sep4;
	TMenuItem *MIN_Import;
	TMenuItem *MIN_ImportCaffePrototxt;
	TOpenDialog *OD_Prototxt;
	TSaveDialog *SD_Prototxt;
	TMenuItem *MIN_Export;
	TMenuItem *MINE_ExportCaffePrototxt;
	TMemo *Me_Evaluation;
	TSplitter *Sp_EvaluationBottom;
	TLayout *Lo_EvaluationDataset;
	TLayout *Lo_Table;
	TMenuItem *MIT_OpenFileLocation;
	TLabel *La_EvaluationData;
	TRadioButton *RB_EvaluationOutputResult;
	TRadioButton *RB_EvaluationConfusionMatrix;
	TComboBox *Co_EvaluationConfusionMatrixModal;
	TImage *Im_CursorForCapture;
	TMenuItem *MILR_RetrainInPlace;
	TMenuItem *MILR_RetrainNotInPlace;
	TMenuItem *MILR_RetrainAll;
	TMenuItem *MILR_Separator4;
	TMenuItem *MIM_Separator0;
	TMenuItem *MIM_SaveCSVAs;
	TSaveDialog *SD_CSV;
	TMenuItem *MIT_SaveCSVAs;
	TPopupMenu *PM_ConfusionMatrix;
	TMenuItem *MIC_SaveCSVAs;
	TOpenDialog *OD_File;
	TLayout *Lo_EditProperty;
	TEdit *Ed_Property;
	TButton *Bu_PropertyBrowse;
	TPopupMenu *PM_Test;
	TTimer *Ti_Animation;
	TMenuItem *MI_TestNaiveNet2Vec;
	TMenuItem *MI_CreateNaiveNet2VecCSVfromLearningResults;
	TPaintBox *PB_Debug;
	TSplitter *Sp_DebugRight;
	TMenuItem *MI_TestGaussianProcessImplementation;
	TMenuItem *MI_TestSimulatedStructureSearchWithNaiveNet2Vec;
	TMenuItem *MI_TestGaussianProcessClosedTest;
	TMenuItem *MI_TestGaussian;
	TMenuItem *MIM_SaveGraphBMPAs;
	TMenuItem *MIM_Separator1;
	TMenuItem *MIM_View;
	TMenuItem *MIMV_TrainingValidation;
	TMenuItem *MIMV_Training;
	TMenuItem *MIMV_Validation;
	TMenuItem *MI_TestSaveGraphBitmaps;
	TMenuItem *MI_TestExportTradeoffCSVForEvaluation;
	TMenuItem *MIN_Zoom;
	TMenuItem *MINZ_Default;
	TMenuItem *MINZ_ZoomIn;
	TMenuItem *MINZ_ZoomOut;
	TMenuItem *MIM_Zoom;
	TMenuItem *MIMZ_Default;
	TMenuItem *MIMZ_ZoomIn;
	TMenuItem *MIMZ_ZoomOut;
	TMenuItem *MI_TestContinueStructureSearch;
	TMenuItem *MI_OnlineDebug;
	TRadioButton *RB_MonitorNormalize;
	TRadioButton *RB_MonitorGain;
	TTrackBar *TB_MonitorGain;
	TLabel *La_MonitorGain;
	TMenuItem *MI_TestAll;
	TMenuItem *MenuItem9;
	TButton *Bu_EvaluationSolve;
	TIdSSLIOHandlerSocketOpenSSL *IdSSLIOHandlerSocketOpenSSL1;
	TMenuItem *MI_Benchmark;
	TMenuItem *MI_RandomZGeneration;
	TPanel *Pa_Splash;
	TLabel *La_Company;
	TLabel *La_SplashVersion;
	TLabel *La_SplashNote;
	TOpenDialog *OD_SDNX;
	TLabel *La_EvaluationDataProgress;
	TLayout *FormLayout;
	TMenuItem *MI_TestDummyImageDataset;
	TMenuItem *MILR_Rename;
	TImage *Im_BackgroundBrain;
	TImage *Im_Save;
	TMenuItem *MIN_ImportSDCPRJ;
	TImage *Im_SongLogo;
	TMenuItem *MINE_PythonCodeNNabla;
	TPaintBox *PB_NetworkTab;
	TMenuItem *MI_FillParameterNameTest;
	TEdit *Ed_NetworkName;
	TMenuItem *MILR_ForceComplete;
	TMemo *Me_Processing;
	TMenuItem *MI_TestAllFull;
	TMenuItem *MI_CompareWithCPU;
	TEdit *Ed_DatasetFileName;
	TFr_List *Fr_DatasetList;
	TLabel *La_DatasetName;
	TCheckBox *Ch_DatasetShuffle;
	TCheckBox *Ch_DatasetEnableCache;
	TCheckBox *Ch_DatasetImageNormalization;
	TCheckBox *Ch_DatasetMain;
	TPopupMenu *PM_DatasetList;
	TMenuItem *MIDL_AddDataset;
	TMenuItem *MIDL_RenameDataset;
	TMenuItem *MIDL_DeleteDataset;
	TMemo *Me_ProjectDescription;
	TLabel *La_StructureSearch;
	TLabel *La_ProjectDescription;
	TEdit *Ed_MaxEpoch;
	TCheckBox *Ch_SaveBest;
	TFr_List *Fr_ConfigList;
	TLabel *La_UpdaterParam5;
	TLabel *La_WeightDecay;
	TLabel *La_UpdaterParam7;
	TLabel *La_UpdaterParam6;
	TLabel *La_UpdaterParam4;
	TLabel *La_UpdaterParam3;
	TLabel *La_UpdaterParam2;
	TLabel *La_UpdaterParam1;
	TLabel *La_Updater;
	TLabel *La_UpdateInterval2;
	TLabel *La_UpdateInterval;
	TEdit *Ed_WeightDecay;
	TEdit *Ed_UpdaterParam7;
	TEdit *Ed_UpdaterParam6;
	TEdit *Ed_UpdaterParam5;
	TEdit *Ed_UpdaterParam4;
	TEdit *Ed_UpdaterParam3;
	TEdit *Ed_UpdaterParam2;
	TEdit *Ed_UpdaterParam1;
	TEdit *Ed_UpdateInterval;
	TComboBox *Co_Updater;
	TLabel *La_NumEvaluations;
	TEdit *Ed_NumEvaluations;
	TLabel *La_OptimizerNetwork;
	TEdit *Ed_OptimizerNetwork;
	TLabel *La_OptimizerDataset;
	TEdit *Ed_OptimizerDataset;
	TLabel *Label6;
	TEdit *Ed_ExecutorNetwork;
	TLabel *Label8;
	TEdit *Ed_MonitorNetwork;
	TEdit *Ed_MonitorDataset;
	TLabel *Label9;
	TPopupMenu *PM_ConfigList;
	TMenuItem *MICL_AddOptimizer;
	TMenuItem *MICL_AddMonitor;
	TMenuItem *MICL_AddExecutor;
	TMenuItem *MICL_Rename;
	TMenuItem *MICL_Delete;
	TLabel *La_BatchSize;
	TEdit *Ed_BatchSize;
	TCheckBox *Ch_ExecutorNeedBackPropagation;
	TRadioButton *RB_ExecutorMean;
	TRadioButton *RB_ExecutorLast;
	TScrollBar *Sc_Property;
	TImage *Im_Warning;
	TMenuItem *MILR_CancelAllScheduledTask;
	TMenuItem *MILR_ResumeScheduledTask;
	TLabel *Label7;
	TEdit *Ed_ExecutorDataset;
	TMenuItem *MILR_CancelSchedule;
	TButton *Bu_AbortProcessing;
	TRectAnimation *RectAnimation1;
	TFr_List *Fr_Browser;
	TSplitter *Sp_NetworkLeft;
	TRectangle *Re_TopBar;
	TLayout *Lo_Right;
	TRectangle *Re_Property;
	TRectangle *Re_Statistics;
	TRectangle *Re_Control;
	TLayout *Lo_Overview;
	TLayout *Lo_Statistics;
	TRectangle *Re_Overview;
	TLabel *La_Overview;
	TRectangle *Re_NetworkTop;
	TButton *Bu_Undo;
	TImage *Im_Undo;
	TButton *Bu_Redo;
	TImage *Im_Redo;
	TButton *Bu_Cut;
	TImage *Im_Cut;
	TButton *Bu_Copy;
	TImage *Im_Copy;
	TButton *Bu_Paste;
	TImage *Im_Paste;
	TFr_List *Fr_ProjectFiles;
	TFr_List *Fr_DatasetFiles;
	TButton *Bu_NetworkTab;
	TButton *Bu_DatasetTab;
	TRectangle *Re_DatasetTab;
	TButton *Bu_ConfigTab;
	TRectangle *Re_ConfigTab;
	TButton *Bu_TrainingTab;
	TRectangle *Re_TrainingTab;
	TButton *Bu_EvaluationTab;
	TRectangle *Re_EvaluationTab;
	TTabControl *TC_Left;
	TTabItem *TI_BrowserLeft;
	TTabItem *TI_NetworkLeft;
	TTabItem *TI_DatasetLeft;
	TTabItem *TI_ConfigLeft;
	TTabItem *TI_TrainingLeft;
	TTabItem *TI_Browser;
	TTabControl *TC_Config;
	TTabItem *TI_GlobalConfig;
	TTabItem *TI_OptimizerConfig;
	TTabItem *TI_MonitorConfig;
	TTabItem *TI_ExecutorConfig;
	TPaintBox *PB_NetworkOverview;
	TPopupMenu *PM_ResultNetwork;
	TMenuItem *MIRN_PlotWeight;
	TMenuItem *MenuItem8;
	TMenuItem *MIRN_OpenInNetworkTab;
	TMenuItem *MIRN_OpenInNetworkTabWithWeight;
	TFr_List *Fr_ResultsList;
	TSplitter *Sp_Right;
	TSplitter *Sp_Left;
	TFr_List *Fr_DatasetFiles2;
	TPopupMenu *PM_List;
	TMenuItem *MIL_OpenFileLocation;
	TMenuItem *MIL_Separator;
	TMenuItem *MIL_Remove;
	TMenuItem *MI_RemoveDeadLinks;
	TRectangle *Re_TrainingTop;
	TRectangle *Re_TrainingTop2;
	TLabel *Ed_MonitorTimeTotal;
	TLabel *La_MonitorTimeTotal;
	TLabel *Ed_MonitorTimeRemaining;
	TLabel *La_MinotorTimeRemaining;
	TLabel *Ed_MonitorTimeElapsed;
	TLabel *La_MonitorTimeElapsed;
	TLabel *La_MonitorEpoch;
	TLabel *La_Epoch;
	TRadioButton *RB_ResultsWeight;
	TRadioButton *RB_ResultsLearningCurve;
	TRadioButton *RB_ResultsTradeoffGraph;
	TComboBox *Co_ResultsTradeoffType;
	TRadioButton *RB_GraphLogScale;
	TRadioButton *RB_GraphLinearScale;
	TRectangle *Re_StatusBar;
	TRectangle *Re_DatasetTop2;
	TRectangle *Re_DatasetTop;
	TRectangle *Re_EvaluationTop;
	TRectangle *Re_EvaluationTop2;
	TScrollBar *Sc_TableY;
	TScrollBar *Sc_Monitor;
	TRectangle *Re_Tasks;
	TLabel *La_Tasks;
	TLabel *La_TaskTraining;
	TLabel *La_TaskEvaluation;
	TProgressBar *PB_TrainingEpoch;
	TProgressBar *PB_EvaluationData;
	TLabel *La_TaskTrainingJob;
	TLabel *La_TaskEvaluationJob;
	TRectangle *Pa_Tasks;
	TFr_List *Fr_Component;
	TImage *Im_Back;
	TButton *Bu_DatasetOpen;
	TImage *Im_Open;
	TButton *Bu_ZoomOut;
	TImage *Image1;
	TButton *Bu_ZoomIn;
	TImage *Image2;
	TButton *Bu_NetworkMenu;
	TImage *Im_Menu;
	TButton *Bu_NewProject;
	TImage *Image3;
	TButton *Bu_OpenProject;
	TImage *Image4;
	TButton *Bu_OpenDataset;
	TImage *Image6;
	TButton *Bu_OpenDataset2;
	TImage *Image7;
	TLayout *Lo_RightSub;
	TRectangle *Re_RightDatasetPreviewTop;
	TLabel *La_DatasetPreview;
	TButton *Bu_Overview;
	TButton *Bu_CreateDataset;
	TImage *Image8;
	TButton *Bu_CreateDataset2;
	TImage *Image9;
	TLabel *La_NetworkZoom;
	TRectangle *Re_NetworkTab;
	TLabel *La_NetworkTab;
	TLabel *La_TrainingTab;
	TLabel *La_EvaluationTab;
	TRectangle *Re_Home;
	TLabel *La_EvaluationTimeElapsed;
	TLabel *Ed_EvaluationTimeRemaining;
	TLabel *La_EvaluationTimeRemaining;
	TLabel *Ed_EvaluationTimeElapsed;
	TLabel *La_EvaluationTimeTotal;
	TLabel *Ed_EvaluationTimeTotal;
	TButton *Bu_ResultZoomOut;
	TImage *Image10;
	TLabel *La_ResultZoom;
	TButton *Bu_ResultZoomIn;
	TImage *Image11;
	TLabel *La_Evaluate;
	TLabel *La_ConfigTab;
	TLabel *La_DatasetTab;
	TImage *Im_Checked;
	TImage *Im_NotChecked;
	TImage *Im_TrainingClock;
	TImage *Image12;
	TImage *Image13;
	TMenuItem *MI_DemoForMovie;
	TRectangle *Re_TopBarLeft;
	TRectangle *Re_NetworkTopLeft;
	TRectangle *Re_TrainingTopLeft;
	TRectangle *Re_TrainingTop2Left;
	TRectangle *Re_EvaluationTopLeft;
	TMenuItem *MI_CompleteTest;
	TMenuItem *MI_LoadAndSaveAllProjects;
	TMenuItem *MI_CreateStructureSearchCSVFile;
	TMenuItem *MILR_OpenInNetworkTab;
	TMenuItem *MILR_OpenInNetworkTabWithWeight;
	TImage *Image14;
	TRectangle *Rectangle1;
	TImage *Image15;
	TImage *Image16;
	TMenuItem *MILR_Refresh;
	TMenuItem *MIN_ImportNnpNntxt;
	TOpenDialog *OD_NnpNntxt;
    TOpenDialog *OD_Nnpt;
	TMenuItem *MILR_Export;
	TMenuItem *MILRE_PPTX;
	TButton *Bu_Help;
	TImage *Im_Help;
	TLabel *La_LearningRateScheduler;
	TComboBox *Co_LearningRateScheduler;
	TTabControl *TC_LearningRateScheduler;
	TTabItem *TI_Exponential;
	TTabItem *TI_Step;
	TEdit *Ed_LearningRateMultiplier;
	TEdit *Ed_LRUpdateInterval;
	TLabel *La_LRUpdateInterval;
	TLabel *La_LearningRateMultiplier;
	TPanel *Pa_StructureSearch;
	TLabel *La_Method;
	TComboBox *Co_StructureSearchMethod;
	TListBoxItem *LBI_Random;
	TListBoxItem *LBI_NetworkFeatureGaussianProcess;
	TComboBox *Co_StructureSearchObjective;
	TListBoxItem *LBI_Error;
	TListBoxItem *LBI_ErrorAndCalculation;
	TLabel *La_StructureSearchOptimizeFor;
	TLabel *La_StructureSearchRange;
	TLabel *La_Validation;
	TLabel *La_MultiplyAdd;
	TEdit *Ed_StructureSearchConstraintCostMultiplyAddMin;
	TEdit *Ed_StructureSearchConstraintCostMultiplyAddMax;
	TLabel *La_TimeLimit;
	TEdit *Ed_TimeLimit;
	TCheckBox *Ch_EarlyStopping;
	TLabel *La_MaxEpoch;
	TEdit *Ed_StructureSearchConstraintValidationMax;
	TEdit *Ed_StructureSearchConstraintValidationMin;
	TLabel *La_StructureSearchMin;
	TLabel *La_StructureSearchMax;
	TCheckBox *Ch_StructureSearchEnabled;
	TLabel *La_TypeConfig;
	TComboBox *Co_TypeConfig;
	TLabel *La_MonitorInterval;
	TEdit *Ed_MonitorInterval;
	TLabel *La_MonitorIntervalUnit;
	TComboBox *Co_LRUpdateIntervalUnit;
	TMenuItem *MIMV_Name;
	TMenuItem *MIMV_Separator0;
	TTabItem *TI_Cosine;
	TTabItem *TI_Polynomial;
	TLabel *Label3;
	TEdit *Ed_StepSchedulerSteps;
	TComboBox *Co_StepSchedulerIntervalUnit;
	TEdit *Ed_PolynomialSchedulerPower;
	TLabel *La_PolynomialSchedulerPower;
	TLabel *Label4;
	TEdit *Ed_StepSchedulerGamma;
	TMenuItem *MILRE_NNP;
	TMenuItem *MILRE_NNB;
	TMenuItem *MILRE_Sep0;
	TMenuItem *MILRE_ONNX;
	TButton *Bu_CloseProcessing;
	TPopupMenu *PM_Statistics;
	TMenuItem *PMS_Copy;
	TMenuItem *MIT_CommandPrompt;
	TMenuItem *MILR_Separator5;
	TMenuItem *MILR_MoreTools;
	TMenuItem *MILRM_CommandPrompt;
	TMenuItem *MIL_Separator2;
	TMenuItem *MIL_MoreTools;
	TMenuItem *MILM_CommandPrompt;
	TButton *Bu_Unitize;
	TImage *Im_Unitize;
	TImage *Image17;
	TMenuItem *MILRE_Html;
	TComboBox *Co_HStatistics;
	TLabel *La_OptimizerRange;
	TEdit *Ed_OptimizerRangeFrom;
	TComboBox *Co_OptimizerRangeUnit;
	TEdit *Ed_OptimizerRangeTo;
	TLabel *La_OptimizerRangeTo;
	TLabel *La_OptimizerRangeFrom;
	TButton *Bu_OverviewPrevious;
	TImage *Im_OverviewPrevious;
	TButton *Bu_OverviewNext;
	TImage *Im_OverviewNext;
	TRadioButton *RB_EvaluationOthers;
	TMenuItem *MINE_Sep0;
	TMenuItem *MINE_pptx;
	TMenuItem *MINE_html;
	TCheckBox *Ch_LearningRateWarmup;
	TEdit *Ed_LearningRateWarmupLength;
	TComboBox *Co_LearningRateWarmupLengthUnit;
	TMenuItem *MIT_Sort;
	TMenuItem *MIC_List;
	TMenuItem *MIC_Sep0;
	TMenuItem *MICL_MoveUp;
	TMenuItem *MICL_MoveDown;
	TMenuItem *MICL_Sep0;
	TMenuItem *MIDL_MoveUp;
	TMenuItem *MIDL_MoveDown;
	TMenuItem *MI_TestRefreshPlugin;
	TMenuItem *MIT_Sep1;
	TMenuItem *MIT_MoreTools;
	TMenuItem *MITM_CommandPrompt;
	TMenuItem *MIT_Plugin;
	TComboBox *Co_EvaluationOthers;
	TRectangle *Re_Processing;
	TComboBox *Co_Instance;
	TComboBox *Co_Training;
	TButton *Bu_Control;
	TRoundRect *RR_ControlDark;
	TRoundRect *RR_ControlLight;
	TLabel *La_Control;
	TMenuItem *MILRE_TensorflowFrozenGraph;
	TMenuItem *MILRE_TensorFlowCheckPoint;
	TMenuItem *MILRE_TensorFlowCheckPointv2;
	TMenuItem *MILR_EditComment;
	TLayout *Lo_OverviewTop;
	TLayout *Lo_OverviewConfig;
	TMemo *Me_OverviewConfig;
	TEdit *Ed_NetworkSearch;
	TLabel *La_NetworkSearch;
	TButton *Button1;
	TImage *Image5;
	TMenuItem *MIN_Replace;
	TMenuItem *MIN_Sep5;
	TMenuItem *MILR_Duplicate;
	TLabel *La_AutoEvaluation;
	TCheckBox *Ch_AutoEvaluation;
	TComboBox *Co_CSVGraphType;
	TLabel *La_CSVGraphType;
	TMediaPlayer *MP_Wav;
	TMenuItem *MI_ClearParameterFiles;
	TLabel *La_NodeX;
	TComboBox *Co_NumNode;
	TLang *Lang;
	TMenuItem *MIN_ImportNnpNntxtasUnit;
    TMenuItem *MIN_ImportNnptasUnit;
    TMenuItem *MIN_ImportNnpt;
	TRadioButton *RB_ExecutorStd;
	TPopupMenu *PM_Components;
	TMenuItem *PMC_RegisterCustomUnit;
	TMenuItem *PMC_DeleteCustomUnit;
	TMenuItem *PMC_Refresh;
	TPopupMenu *PM_CreateDataset;
	TMenuItem *MICD_ImageClassification;
	TMenuItem *MILRE_TensorFlowLite;
	TLayout *Lo_PropertyLeft;
	TSplitter *Sp_Property;
	TLabel *La_OutOfCoreTraining;
	TCheckBox *Ch_OutOfCoreTrainingEnabled;
	TLabel *La_OutOfCoreTrainingGPUMemorySize;
	TEdit *Ed_OutOfCoreTrainingGPUMemorySize;
	TLabel *La_OutOfCoreTrainingWindowSize;
	TEdit *Ed_OutOfCoreTrainingWindowSize;
	TLabel *La_OutOfCoreTrainingGPUMemorySizeByte;
	TLabel *La_OutOfCoreTrainingWindowSizeByte;
	TMenuItem *MILR_ResumeTraining;
	TLabel *La_UpdaterParam8;
	TEdit *Ed_UpdaterParam8;
	TLabel *La_UpdaterParam9;
	TEdit *Ed_UpdaterParam9;
	TMenuItem *MIT_WeightCheck;
	TMenuItem *MIMV_None;
	TMenuItem *MIMV_Comment;
	TMenuItem *MIM_Separator2;
	TMenuItem *MIM_OpenInNetworkTab;
	TMenuItem *MIM_OpenInNetworkTabWithWeight;
	TButton *Bu_NetworkTab2;
	TLayout *Lo_MonitorBottom;
	TMemo *Me_Monitor;
	TButton *Bu_MonitorSolve;
	TRectangle *Re_MonitorLogTop;
	TRadioButton *RB_MonitorLog;
	TRadioButton *RB_MonitorCPU;
	TPaintBox *PB_MonitorPerformance;
	TRadioButton *RB_MonitorGPU;
	TButton *Bu_MonitorPerformance;
	TButton *Bu_Monitor;
	TLayout *Lo_CSVGraphType;
	TFr_List *Fr_NNablaExamples;
	TLabel *La_InteractiveDemos;
	TRectangle *Re_RightBlankTop;
	TMenuItem *MIT_PluginDataset;
	TMenuItem *MIC_Recreate;
	TButton *Bu_NewProjectTransformers;
	TImage *Image18;
	TComboBox *Co_Run;
	TPopupMenu *PM_NewProjectTransformers;
	TMenuItem *MINPT_EmptyProject;
	TMenuItem *MINPT_Sep;
	TLayout *Lo_ControlLine1;
	TLayout *Lo_ControlLine2;
	TLayout *Lo_DataPreview;
	TRectangle *Rectangle2;
	TLabel *La_DataPreview;
	TPaintBox *PB_DataPreview;
	TMenuItem *MICD_FromText;
	TFr_List *Fr_AISolutions;
	TLabel *La_AISolutions;
	TMenuItem *MIT_OpenTable;
	TButton *Bu_NewProjectTorch;
	TImage *Image19;
	void __fastcall FormShow(TObject *Sender);
	void __fastcall Bu_SaveClick(TObject *Sender);
	void __fastcall Bu_SaveAsClick(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall Ti_AnimationTimer(TObject *Sender);
	void __fastcall PB_NetworkPaint(TObject *Sender, TCanvas *Canvas);
	void __fastcall PB_NetworkMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift,
          float X, float Y);
	void __fastcall Bu_NetworkKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift);
	void __fastcall PB_NetworkMouseUp(TObject *Sender, TMouseButton Button, TShiftState Shift,
          float X, float Y);
	void __fastcall PB_NetworkMouseMove(TObject *Sender, TShiftState Shift, float X,
		  float Y);
	void __fastcall PB_NetworkDblClick(TObject *Sender);
	void __fastcall PB_PropertyPaint(TObject *Sender, TCanvas *Canvas);
	void __fastcall PB_PropertyMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift,
          float X, float Y);
	void __fastcall TV_ComponentClick(TObject *Sender);
	void __fastcall Ed_PropertyKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift);
	void __fastcall Bu_PropertyKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift);
	void __fastcall FormKeyUp(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift);
	void __fastcall MIN_SelectAllClick(TObject *Sender);
	void __fastcall MIN_ClearSelectionClick(TObject *Sender);
	void __fastcall MIN_DeleteClick(TObject *Sender);
	void __fastcall PB_StatisticsPaint(TObject *Sender, TCanvas *Canvas);
	void __fastcall PB_StatisticsMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift,
          float X, float Y);
	void __fastcall Bu_StatisticsKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift);
	void __fastcall MIN_ArrangeLayersClick(TObject *Sender);
	void __fastcall FormResize(TObject *Sender);
	void __fastcall Bu_DatasetOpenClick(TObject *Sender);
	void __fastcall PB_TablePaint(TObject *Sender, TCanvas *Canvas);
	void __fastcall Bu_TableKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift);
	void __fastcall PB_TableMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift,
		  float X, float Y);
	void __fastcall PB_TableMouseWheel(TObject *Sender, TShiftState Shift, int WheelDelta,
          bool &Handled);
	void __fastcall Sc_TableYChange(TObject *Sender);
	void __fastcall Bu_BackClick(TObject *Sender);
	void __fastcall FA_TCDefaultPositionXFinish(TObject *Sender);
	void __fastcall MIL_RemoveClick(TObject *Sender);
	void __fastcall Ed_BatchSizeChange(TObject *Sender);
	void __fastcall Ed_MaxEpochChange(TObject *Sender);
	void __fastcall Ed_UpdaterParam1Change(TObject *Sender);
	void __fastcall Ed_PropertyChangeTracking(TObject *Sender);
	void __fastcall MIL_OpenFileLocationClick(TObject *Sender);
	void __fastcall PB_MonitorPaint(TObject *Sender, TCanvas *Canvas);
	void __fastcall Bu_SetupClick(TObject *Sender);
	void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
	void __fastcall Co_PropertyKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift);
	void __fastcall Co_PropertyChange(TObject *Sender);
	void __fastcall FormKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
		  TShiftState Shift);
	void __fastcall MIM_OpenResultLocationClick(TObject *Sender);
	void __fastcall Bu_UndoClick(TObject *Sender);
	void __fastcall Bu_RedoClick(TObject *Sender);
	void __fastcall MI_UndoClick(TObject *Sender);
	void __fastcall MIN_InverseSelectionClick(TObject *Sender);
	void __fastcall Bu_CutClick(TObject *Sender);
	void __fastcall Bu_CopyClick(TObject *Sender);
	void __fastcall Bu_PasteClick(TObject *Sender);
	void __fastcall Sc_TableYExit(TObject *Sender);
	void __fastcall Sc_NetworkYChange(TObject *Sender);
	void __fastcall Sc_NetworkYExit(TObject *Sender);
	void __fastcall Sc_NetworkXChange(TObject *Sender);
	void __fastcall Sc_NetworkXExit(TObject *Sender);
	void __fastcall Sc_TableXChange(TObject *Sender);
	void __fastcall PB_NetworkMouseWheel(TObject *Sender, TShiftState Shift, int WheelDelta,
          bool &Handled);
	void __fastcall MIT_OpenDataClick(TObject *Sender);
	void __fastcall Me_MonitorDblClick(TObject *Sender);
	void __fastcall TC_DefaultChange(TObject *Sender);
	void __fastcall MILR_DeleteFromDiskClick(TObject *Sender);
	void __fastcall MILR_DeleteAllIncompleteResultsFromDiskClick(TObject *Sender);
	void __fastcall Ch_StructureSearchEnabledChange(TObject *Sender);
	void __fastcall MILR_OpenLearningCurveForComparisonClick(TObject *Sender);
	void __fastcall MILR_ClearLearningCurveForComparisonClick(TObject *Sender);
	void __fastcall MIN_SaveNetworkBitmapAsClick(TObject *Sender);
	void __fastcall PB_NetworkOverviewPaint(TObject *Sender, TCanvas *Canvas);
	void __fastcall MIRN_OpenInNetworkTabClick(TObject *Sender);
	void __fastcall Me_MonitorClick(TObject *Sender);
	void __fastcall Me_MonitorMouseWheel(TObject *Sender, TShiftState Shift, int WheelDelta,
          bool &Handled);
	void __fastcall Me_MonitorKeyUp(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift);
	void __fastcall FA_HintFinish(TObject *Sender);
	void __fastcall Ed_DatasetFileNameDblClick(TObject *Sender);
	void __fastcall PB_NetworkOverviewDblClick(TObject *Sender);
	void __fastcall Bu_FullScreenClick(TObject *Sender);
	void __fastcall PB_MonitorClick(TObject *Sender);
	void __fastcall MIT_CheckConsistencyClick(TObject *Sender);
	void __fastcall PB_MonitorMouseMove(TObject *Sender, TShiftState Shift, float X,
          float Y);
	void __fastcall PB_MonitorMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift,
		  float X, float Y);
	void __fastcall MIN_ImportCaffePrototxtClick(TObject *Sender);
	void __fastcall MINE_ExportCaffePrototxtClick(TObject *Sender);
	void __fastcall Ed_LearningRateMultiplierChange(TObject *Sender);
	void __fastcall MIT_OpenFileLocationClick(TObject *Sender);
	void __fastcall Co_EvaluationConfusionMatrixModalChange(TObject *Sender);
	void __fastcall Me_EvaluationDblClick(TObject *Sender);
	void __fastcall RB_EvaluationOutputResultChange(TObject *Sender);
	void __fastcall Ed_StructureSearchConstraintValidationMinChange(TObject *Sender);
	void __fastcall Ed_StructureSearchConstraintValidationMaxChange(TObject *Sender);
	void __fastcall Ed_StructureSearchConstraintCostMultiplyAddMinChange(TObject *Sender);
	void __fastcall Ed_StructureSearchConstraintCostMultiplyAddMaxChange(TObject *Sender);
	void __fastcall MILR_RetrainNotInPlaceClick(TObject *Sender);
	void __fastcall MILR_RetrainInPlaceClick(TObject *Sender);
	void __fastcall MILR_RetrainAllClick(TObject *Sender);
	void __fastcall MIM_SaveCSVAsClick(TObject *Sender);
	void __fastcall MIT_SaveCSVAsClick(TObject *Sender);
	void __fastcall Ch_EarlyStoppingChange(TObject *Sender);
	void __fastcall Ed_TimeLimitChange(TObject *Sender);
	void __fastcall Bu_PropertyBrowseClick(TObject *Sender);
	void __fastcall MI_TestNaiveNet2VecClick(TObject *Sender);
	void __fastcall MI_CreateNaiveNet2VecCSVfromLearningResultsClick(TObject *Sender);
	void __fastcall PB_DebugPaint(TObject *Sender, TCanvas *Canvas);
	void __fastcall MI_TestGaussianProcessImplementationClick(TObject *Sender);
	void __fastcall MI_TestSimulatedStructureSearchWithNaiveNet2VecClick(TObject *Sender);
	void __fastcall Co_StructureSearchMethodChange(TObject *Sender);
	void __fastcall MI_TestGaussianProcessClosedTestClick(TObject *Sender);
	void __fastcall MI_TestGaussianClick(TObject *Sender);
	void __fastcall MIM_SaveGraphBMPAsClick(TObject *Sender);
	void __fastcall MIMV_TrainingValidationClick(TObject *Sender);
	void __fastcall MI_TestSaveGraphBitmapsClick(TObject *Sender);
	void __fastcall MI_TestExportTradeoffCSVForEvaluationClick(TObject *Sender);
	void __fastcall MINZ_DefaultClick(TObject *Sender);
	void __fastcall MINZ_ZoomInClick(TObject *Sender);
	void __fastcall MINZ_ZoomOutClick(TObject *Sender);
	void __fastcall Bu_MonitorSolveClick(TObject *Sender);
	void __fastcall PB_MonitorMouseWheel(TObject *Sender, TShiftState Shift, int WheelDelta,
          bool &Handled);
	void __fastcall MIMZ_DefaultClick(TObject *Sender);
	void __fastcall MIMZ_ZoomInClick(TObject *Sender);
	void __fastcall MIMZ_ZoomOutClick(TObject *Sender);
	void __fastcall MI_TestContinueStructureSearchClick(TObject *Sender);
	void __fastcall Co_StructureSearchObjectiveChange(TObject *Sender);
	void __fastcall Me_MonitorKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift);
	void __fastcall MI_OnlineDebugClick(TObject *Sender);
	void __fastcall MIRN_OpenInNetworkTabWithWeightClick(TObject *Sender);
	void __fastcall PB_NetworkOverviewMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, float X, float Y);
	void __fastcall Sc_MonitorChange(TObject *Sender);
	void __fastcall Sc_MonitorExit(TObject *Sender);
	void __fastcall RB_MonitorNormalizeClick(TObject *Sender);
	void __fastcall TB_MonitorGainChange(TObject *Sender);
	void __fastcall MIRN_PlotWeightClick(TObject *Sender);
	void __fastcall MI_TestAllClick(TObject *Sender);
	void __fastcall Bu_EvaluationSolveClick(TObject *Sender);
	void __fastcall MI_BenchmarkClick(TObject *Sender);
	void __fastcall MI_RandomZGenerationClick(TObject *Sender);
	void __fastcall Ch_SaveBestChange(TObject *Sender);
	void __fastcall Ed_PropertyDblClick(TObject *Sender);
	void __fastcall MI_RemoveDeadLinksClick(TObject *Sender);
	void __fastcall MI_TestDummyImageDatasetClick(TObject *Sender);
	void __fastcall MILR_RenameClick(TObject *Sender);
	void __fastcall MIN_ImportSDCPRJClick(TObject *Sender);
	void __fastcall MINE_PythonCodeNNablaClick(TObject *Sender);
	void __fastcall PB_NetworkTabPaint(TObject *Sender, TCanvas *Canvas);
	void __fastcall Ed_UpdateIntervalChange(TObject *Sender);
	void __fastcall Ed_LRUpdateIntervalChange(TObject *Sender);
	void __fastcall Co_UpdaterChange(TObject *Sender);
	void __fastcall Ed_WeightDecayChange(TObject *Sender);
	void __fastcall PB_NetworkTabMouseMove(TObject *Sender, TShiftState Shift, float X,
          float Y);
	void __fastcall MILR_ForceCompleteClick(TObject *Sender);
	void __fastcall Ed_NumEvaluationsChange(TObject *Sender);
	void __fastcall MI_CompareWithCPUClick(TObject *Sender);
	void __fastcall Fr_DatasetListBu_ListKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift);
	void __fastcall Ch_DatasetMainClick(TObject *Sender);
	void __fastcall MIDL_DeleteDatasetClick(TObject *Sender);
	void __fastcall MIDL_AddDatasetClick(TObject *Sender);
	void __fastcall MIDL_RenameDatasetClick(TObject *Sender);
	void __fastcall Fr_DatasetListSc_ListYChange(TObject *Sender);
	void __fastcall Ed_OptimizerNetworkChange(TObject *Sender);
	void __fastcall Ed_OptimizerDatasetChange(TObject *Sender);
	void __fastcall Ed_MonitorNetworkChange(TObject *Sender);
	void __fastcall Ed_MonitorDatasetChange(TObject *Sender);
	void __fastcall Ed_ExecutorNetworkChange(TObject *Sender);
	void __fastcall MICL_RenameClick(TObject *Sender);
	void __fastcall MICL_AddOptimizerClick(TObject *Sender);
	void __fastcall MICL_AddMonitorClick(TObject *Sender);
	void __fastcall MICL_AddExecutorClick(TObject *Sender);
	void __fastcall MICL_DeleteClick(TObject *Sender);
	void __fastcall Ch_ExecutorNeedBackPropagationChange(TObject *Sender);
	void __fastcall Ch_DatasetShuffleChange(TObject *Sender);
	void __fastcall Ch_DatasetEnableCacheChange(TObject *Sender);
	void __fastcall Ch_DatasetImageNormalizationChange(TObject *Sender);
	void __fastcall RB_ExecutorStdChange(TObject *Sender);
	void __fastcall RB_ExecutorLastChange(TObject *Sender);
	void __fastcall Sc_PropertyExit(TObject *Sender);
	void __fastcall Sc_PropertyChange(TObject *Sender);
	void __fastcall MILR_CancelAllScheduledTaskClick(TObject *Sender);
	void __fastcall MILR_ResumeScheduledTaskClick(TObject *Sender);
	void __fastcall Ed_ExecutorDatasetChange(TObject *Sender);
	void __fastcall MILR_CancelScheduleClick(TObject *Sender);
	void __fastcall Bu_AbortProcessingClick(TObject *Sender);
	void __fastcall Bu_NetworkTabClick(TObject *Sender);
	void __fastcall Bu_DatasetTabClick(TObject *Sender);
	void __fastcall Bu_ConfigTabClick(TObject *Sender);
	void __fastcall Bu_TrainingTabClick(TObject *Sender);
	void __fastcall Bu_EvaluationTabClick(TObject *Sender);
	void __fastcall RB_GraphLinearScaleChange(TObject *Sender);
	void __fastcall Pa_TasksResize(TObject *Sender);
	void __fastcall Re_DatasetTopResize(TObject *Sender);
	void __fastcall PB_NetworkResize(TObject *Sender);
	void __fastcall PB_TableResize(TObject *Sender);
	void __fastcall Bu_NetworkMenuClick(TObject *Sender);
	void __fastcall Bu_NewProjectClick(TObject *Sender);
	void __fastcall Bu_OpenProjectClick(TObject *Sender);
	void __fastcall Bu_OpenDatasetClick(TObject *Sender);
	void __fastcall Bu_OpenDataset2Click(TObject *Sender);
	void __fastcall Fr_ProjectFilesResize(TObject *Sender);
	void __fastcall Fr_DatasetFilesResize(TObject *Sender);
	void __fastcall Fr_DatasetFiles2Resize(TObject *Sender);
	void __fastcall TC_LeftResize(TObject *Sender);
	void __fastcall RB_ResultsLearningCurveChange(TObject *Sender);
	void __fastcall Bu_CreateDatasetClick(TObject *Sender);
	void __fastcall Lo_PropertyResize(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall MI_DemoForMovieClick(TObject *Sender);
	void __fastcall MI_CompleteTestClick(TObject *Sender);
	void __fastcall MI_FillParameterNameTestClick(TObject *Sender);
	void __fastcall MI_LoadAndSaveAllProjectsClick(TObject *Sender);
	void __fastcall MI_CreateStructureSearchCSVFileClick(TObject *Sender);
	void __fastcall Me_ProjectDescriptionChangeTracking(TObject *Sender);
	void __fastcall MILR_RefreshClick(TObject *Sender);
	void __fastcall MIN_ImportNnpNntxtClick(TObject *Sender);
	void __fastcall MIN_ImportNnptClick(TObject *Sender);
	void __fastcall MILRE_ReportClick(TObject *Sender);
	void __fastcall Bu_HelpClick(TObject *Sender);
	void __fastcall Co_LearningRateSchedulerChange(TObject *Sender);
	void __fastcall Co_TypeConfigChange(TObject *Sender);
	void __fastcall Ed_MonitorIntervalChangeTracking(TObject *Sender);
	void __fastcall Co_LRUpdateIntervalUnitChange(TObject *Sender);
	void __fastcall MIMV_NameClick(TObject *Sender);
	void __fastcall Ed_PolynomialSchedulerPowerChangeTracking(TObject *Sender);
	void __fastcall Ed_StepSchedulerGammaChangeTracking(TObject *Sender);
	void __fastcall Co_StepSchedulerIntervalUnitChange(TObject *Sender);
	void __fastcall Ed_StepSchedulerStepsChangeTracking(TObject *Sender);
	void __fastcall MILRE_NNPClick(TObject *Sender);
	void __fastcall Bu_CloseProcessingClick(TObject *Sender);
	void __fastcall PMS_CopyClick(TObject *Sender);
	void __fastcall MIT_CommandPromptClick(TObject *Sender);
	void __fastcall MILRM_CommandPromptClick(TObject *Sender);
	void __fastcall Bu_UnitizeClick(TObject *Sender);
	void __fastcall Co_HStatisticsExit(TObject *Sender);
	void __fastcall Co_HStatisticsChange(TObject *Sender);
	void __fastcall Ed_OptimizerRangeFromChangeTracking(TObject *Sender);
	void __fastcall Ed_OptimizerRangeToChangeTracking(TObject *Sender);
	void __fastcall Co_OptimizerRangeUnitChange(TObject *Sender);
	void __fastcall Lo_OverviewResize(TObject *Sender);
	void __fastcall Bu_OverviewPreviousClick(TObject *Sender);
	void __fastcall Ed_LearningRateWarmupLengthChangeTracking(TObject *Sender);
	void __fastcall Co_LearningRateWarmupLengthUnitChange(TObject *Sender);
	void __fastcall Ch_LearningRateWarmupChange(TObject *Sender);
	void __fastcall MIDL_MoveUpClick(TObject *Sender);
	void __fastcall MIDL_MoveDownClick(TObject *Sender);
	void __fastcall MICL_MoveUpClick(TObject *Sender);
	void __fastcall MICL_MoveDownClick(TObject *Sender);
	void __fastcall PB_NetworkTabMouseUp(TObject *Sender, TMouseButton Button, TShiftState Shift,
          float X, float Y);
	void __fastcall PB_NetworkTabMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift,
          float X, float Y);
	void __fastcall Ed_DatasetFileNameClick(TObject *Sender);
	void __fastcall FormMouseWheel(TObject *Sender, TShiftState Shift, int WheelDelta,
          bool &Handled);
	void __fastcall Co_EvaluationOthersChange(TObject *Sender);
	void __fastcall Co_TrainingChange(TObject *Sender);
	void __fastcall Bu_ControlClick(TObject *Sender);
	void __fastcall Co_InstanceChange(TObject *Sender);
	void __fastcall MILR_EditCommentClick(TObject *Sender);
	void __fastcall Ed_NetworkSearchKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift);
	void __fastcall Ed_NetworkSearchChangeTracking(TObject *Sender);
	void __fastcall MIN_ReplaceClick(TObject *Sender);
	void __fastcall MILR_DuplicateClick(TObject *Sender);
	void __fastcall Ch_AutoEvaluationChange(TObject *Sender);
	void __fastcall Co_CSVGraphTypeChange(TObject *Sender);
	void __fastcall MI_ClearParameterFilesClick(TObject *Sender);
	void __fastcall MITM_CommandPromptClick(TObject *Sender);
	void __fastcall Fr_ComponentBu_ListKeyDown(TObject *Sender, WORD &Key, System::WideChar &KeyChar,
          TShiftState Shift);
	void __fastcall RB_ExecutorMeanChange(TObject *Sender);
	void __fastcall PMC_RefreshClick(TObject *Sender);
	void __fastcall PMC_RegisterCustomUnitClick(TObject *Sender);
	void __fastcall PMC_DeleteCustomUnitClick(TObject *Sender);
	void __fastcall MICD_ImageClassificationClick(TObject *Sender);
	void __fastcall Lo_PropertyLeftResized(TObject *Sender);
	void __fastcall Ch_OutOfCoreTrainingEnabledChange(TObject *Sender);
	void __fastcall Ed_OutOfCoreTrainingGPUMemorySizeChangeTracking(TObject *Sender);
	void __fastcall Ed_OutOfCoreTrainingWindowSizeChangeTracking(TObject *Sender);
	void __fastcall MILR_ResumeTrainingClick(TObject *Sender);
	void __fastcall MIT_WeightCheckClick(TObject *Sender);
	void __fastcall PB_NetworkOverviewMouseMove(TObject *Sender, TShiftState Shift,
          float X, float Y);
	void __fastcall PB_MonitorPerformancePaint(TObject *Sender, TCanvas *Canvas);
	void __fastcall PB_MonitorPerformanceMouseMove(TObject *Sender, TShiftState Shift,
          float X, float Y);
	void __fastcall RB_MonitorCPUClick(TObject *Sender);
	void __fastcall Bu_OverviewExit(TObject *Sender);
	void __fastcall Bu_MonitorPerformanceExit(TObject *Sender);
	void __fastcall PB_MonitorPerformanceClick(TObject *Sender);
	void __fastcall Fr_NNablaExamplesResize(TObject *Sender);
	void __fastcall MIC_RecreateClick(TObject *Sender);
	void __fastcall MINPT_EmptyProjectClick(TObject *Sender);
	void __fastcall PB_DataPreviewPaint(TObject *Sender, TCanvas *Canvas);
	void __fastcall MICD_FromTextClick(TObject *Sender);
	void __fastcall MIT_OpenTableClick(TObject *Sender);
	void __fastcall Bu_NewProjectTransformersClick(TObject *Sender);
	void __fastcall Bu_NewProjectTorchClick(TObject *Sender);






public:
	explicit __fastcall TFo_Main(TComponent* Owner);
	unsigned int time_created_;

public:
    void RefreshLayerComponents();
	int RefreshLayerComponentTree(SDeepEngineType sdeep_engine_type);
    void ComponentMouseDown(TMouseButton button, TShiftState shift, float X, float Y);
	void ComponentMouseMove(TMouseButton button, TShiftState shift, float X, float Y);
	void ComponentMouseUp(TMouseButton button, TShiftState shift, float X, float Y);
private:
	FListT<TTreeViewItem*> nn_layer_component_items_;

	SDeepConsoleGlobalSettings settings_;
	FFastIni *setting_file_;

private:
	//Acrivity
	SDeepConsoleActivity activity_;
	void SetActivity(SDeepConsoleActivity activity);

private:
	//Document
	void NewProject(SDeepEngineType sdeep_engine_type = SDeepEngineType(0), bool need_refresh_component = true);
	int OpenProject(UnicodeString file_name, int preset_project = false);
	int RegisterProject(UnicodeString file_name);
	int Save();
	int SaveAs();
	int CheckSave();

	int ApplyProperty();

	UnicodeString GetResultDir(int result_index = -1);
	FString GetCurrentFileLocation();
	FString GetCurrentImageFileName();
private:
	NNDocument nn_document_;
	NNDocument nn_document_tmp_;//for scheduled training etc.

	ListItems project_list_;
	ListItems dataset_list_;
	int update_count_;
	int view_update_count_;

	void SetupPresetProjectMenu();
	void __fastcall PresetProjectClick(TObject *Sender);
private:
	//Top Buttons
	int AddHint(TControl *control, FString hint);
	FListT<TControl*> controls_;
	FStringList control_hints_;
private:
	void RefreshListSettings();
	void BrowserItemChanged(int index); //Callback from Fr_List
	void RefreshProjectFiles();
	void ProjectsItemChanged(int index); //Callback from Fr_List
	void ProjectsItemClick(int index); //Callback from Fr_List
	void RefreshDatasetFiles();
	void DatasetsItemChanged(int index); //Callback from Fr_List
	void Datasets2ItemClick(int index); //Callback from Fr_List
	void RefreshAISolutionsList();
	void AISolutionsItemClick(int index); //Callback from Fr_List
	void RefreshNNablaExamplesList();
	void NNablaExamplesItemClick(int index); //Callback from Fr_List

private:
	ControlButtonType control_button_type_;
private:
	//Network GUI
	NNGraphMapper nn_graph_mapper_;
	float sdeep_console_transition_coef_;
	void DrawNetworkBitmap(TCanvas *C, int width, int height, NNGraphMapper &graph_mapper, double current_left = 0.0, double current_top = 0.0, double zoom = 1.0);
	void SaveNetworkBitmapToFile(NNDocument &nn_document, UnicodeString file_name);
private:
	void ComponentDblClick(int index);
	FListT<int> GetComponentIndex();
	void Refresh();

private:
	int network_focuse_index_;//0=Component, 1=Network, 2=Property

	FListT<TRectF> use_button_rect_;

	ComponentDragMode component_drag_mode_;
	TPointF component_last_drag_pos_;

	double network_left_, network_current_left_;
	double network_top_, network_current_top_;
	int network_zoom_;
	double network_current_zoom_;
	int network_mouse_down_;
	NetworkMouseDownMode network_mouse_down_mode_;
	float network_start_x_;
	float network_start_y_;
	float network_last_x_;
	float network_last_y_;
    int last_layer_id_;

	float property_top_;
	float property_current_top_;
	int property_current_pos_;
	int visible_property_num_;
	int EditProperty();
	void ClearEditBox();
	int current_property_index_;
	int current_property_layer_id_;
	FListT<TRectF> property_rect_;
	FListT<TRectF> property_value_rect_;

	FListT<TRectF> statistics_rect_;

private:
	//Network Tab
	int network_tab_index_2_;
	int network_tab_delete_focus_;
	FStringList network_tab_names_;
	FListT<TRectF> network_tab_rect_;
	FListT<TRectF> network_tab_delete_rect_;
	TRectF network_tab_add_rect_;

	int drag_network_tab_;
	int drag_network_tab_start_x_;
	int drag_network_tab_x_shift_;

	//Instance
	FString instance_list_;
	FListT<int> instance_type_;// ssh:0x7f000000 + n
	FListT<int> instance_available_;
	FListP<FListT<int> > instance_num_node_;
	void OpenInstanceList();
	void UpdateInstanceList(FString process = FString("train"));

	NNLayer* AddLayer(int auto_link = false, float x = -1.0f, float y = -1.0f, int escape = true, int insert = false);
	void SearchLayer(int select_next); //select_next = -1(select previous), 0, or 1(select next)
private:
	//Dataset GUI
	void RefreshDatasetList();
	void DatasetItemChanged(int index); //Callback from Fr_List
	void DatasetItemRename(int index, FString text); //Callback from Fr_List
	void DatasetItemDelete(int index); //Callback from Fr_List

	int default_dataset_col_width_;
	int default_dataset_row_height_;

	FListT<const CSVDataset *> table_ptrs_;
	FListP<TableUIState> table_ui_state_;
	ImageCacheList *image_cache_list_;
	CSVCacheList *csv_cache_list_;

	void OpenDataset(int dataset_index, UnicodeString file_name, int update_dataset_list = true);
	int RegisterAndOpenNewDataset(int dataset_index);
	int RegisterDatasetCSVs(FString out_dir);

	void SetDatasetCursorRect();
	void PreviewData(int clear = false);

	int IsFile(UnicodeString file_name);
	int IsCSVFile(UnicodeString file_name);
	int IsImageFile(UnicodeString file_name);
	int IsUnknownSizeFile(UnicodeString file_name);
    int IsWavFile(UnicodeString file_name);
	UnicodeString GetDataFileName(UnicodeString path, UnicodeString file_name);
	void DrawDataCell(TCanvas *C, TRectF rect, UnicodeString data_text, FStringList &request_images, FStringList &request_csvs, FListT<CSVCacheFileType> &request_csv_types);

	int dataset_update_count_;
	int dataset_internal_update_count_;
	int dataset_image_update_count_;
	int dataset_csv_update_count_;
	OthersTableType others_table_type_;
	int dataset_header_height_;
	int dataset_header_width_;
	int csv_graph_exists_;// to change Lo_CSVGraphType->Visible
    int show_dataset_plugin_result_;//display dataset plugin result table

	const CSVDataset &GetCurrentTable();
	TableUIState &GetCurrentTableUIState();
	void SetCSVGraphType();
private:
	//Config GUI
	void RefreshConfigList();
	void RefreshConfigTab(int index);
	void ConfigItemDelete(int index); //Callback from Fr_List
	void ConfigItemRename(int index, FString name); //Callback from Fr_List
	FListT<ConfigType> config_type_;

private:
	//Training Results GUI (Monitor)
	int DisplayDocumentError();
	int CheckDataSizeError();
	int CheckDataSizeErrorSub(FString network_name, const FStringList &dataset_names);
	int GetDatasetVariable(const NNDataset &dataset, FStringList &variable_names, FListP<FListT<int> >  &sizes, FListT<int> &types, FStringList &errors);
	int CheckDatasetConsistencyWithNetwork(const NNDataset &dataset, const NNStructure &structure, FStringList &errors);
	int GetDataSizeAndType(const NNDataset &dataset, int row_index, int col_index, FListT<int> &size, int &type, FStringList &errors);
	int PrepareTraining();
	void ExecuteTraining(NNDocument &document, int resume = false);
    int TrainNext();//Continue to scheduled job and structure search
	void ScheduleTraining();
	int ExecuteEvaluation(const FString &name, const FString &result_dir, int auto_evaluation);
	void FreeSDeepTrainingThread();
	int AskAndTerminateProcess();

	void TrainingResultRename(int index, FString name);

	FListT<SDeepTrainingThread*> sdeep_training_thread_;
	FListP<NNError> current_errors_;

	NNDocument result_network_;
	FString result_name_[2];
	FString result_network_name_;
	CSVDataset monitor_csv_[2];
	int monitor_update_count_[2];
	int monitor_date_, monitor_update_request_;
	double monitor_value_range_[6];//(epoch, cost, train/valid) * (min, max)
	double monitor_current_value_range_[6];//(epoch, cost, train/valid) * (min, max)
	int monitor_zoom_;
    TRectF monitor_checkbox_[2];

	CSVDataset tradeoff_csv_;
	int result_tradeoff_update_count_;
	double result_tradeoff_value_range_[4];//(nummultiplyadd, train/valid) * (min, max)
	double result_current_tradeoff_value_range_[4];//(nummultiplyadd, train/valid) * (min, max)
	FString result_tradeoff_h_statistics_str_;
	TRectF result_tradeoff_h_statistics_rect_;
	int result_tradeoff_zoom_;
	int result_last_mouse_down_;
	float result_last_mouse_x_;
	float result_last_mouse_y_;

	UnicodeString log_file_name_;
	//TrainingResultGraphMode training_result_graph_mode_;

	NNWeight result_weight_;
	int result_weight_zoom_;
	double result_weight_current_zoom_;
	double result_weight_top_;
	double result_weight_current_top_;

	float monitor_performance_last_x_, monitor_performance_last_y_;

	void ResetMonitor(int all = true);
	void RefreshMonitorTimeFromLog();
	void OpenResultsNNtxt(AnsiString result_network_file_name, NNDocument &nn_document, int finalize = true);
	void OpenResult(int result_index = -1, int for_comparison = false);
	void RefreshOtherFilesInResultDir(int result_index, FString active_file_name);//Refresh Others popup-box in the Evaluation Tab
	void RefreshResultsList(int reset_position = true);
	void ResultsListDblClick(int index);
	void ResultsListDelete(int index);
	int OpenNetworkFromIni(NNStructures &networks, UnicodeString network_file_name);
	int JumpToError(int edit);
	UnicodeString training_monitor_solve_dir_;//lock file dir to delete

	int TrainScheduled();
	void SetNNablaWeight(FString result_dir, NNDocument &document);// set weight properties with folder containing weight txt files
	void SetTorchWeight(FString result_dir, NNDocument &document);
	int OpenNetworkFromResult(int load_weight);//read network.ini / network.xml(load_weight=false) or net.sdn / net.sdnx(load_weight=true)
	int OpenSDNX(FString sdnx_file_name, int load_weight);
	void ExtractNetwork(FString net_file_name_no_ext, FString output_dir, int overwrite);//Create unziped net at "\\net" folder from net.sdn or net.sdnx

	void DrawGraphBitmap(TCanvas *C, int width, int height, FString type);
	void DrawGraphBitmap_LearningCurve(TCanvas *C, int width, int height);
	void DrawGraphBitmap_TradeoffGraph(TCanvas *C, int width, int height);
	void DrawGraphBitmap_Weight(TCanvas *C, int width, int height);
	void SaveGraphBitmapToFile(UnicodeString file_name, int width = 1280, int height = 720, const FString &type = FString());
private:
	//Evaluation GUI
	void FreeSDeepEvaluationThread();
	SDeepEvaluationThread *sdeep_evaluation_thread_;

	void ResetEvaluationResult();
	void RefreshEvaluationTimeFromLog();
	void OpenEvaluationResult(int need_reload, int need_result);
	void UpdateConfusionMatrix(int force = false, double binary_threshold = 0.5);
	int SearchVariableInDataset(const FString &variable_name, int &dim_size, int &col_start_index, int &num_category, int &by_index);
	void CreateOutputIndexSeries(const FString &variable_name, int result, FListT<int> *indices, double binary_threshold = 0.5);
	void ErrorAnalysis(const FString &label, const FString &result, int label_index, int result_index, double binary_threshold = 0.5);
	void SortOutputResultByValue(int col_index);

	UnicodeString evaluation_solve_dir_;//lock file dir to delete

private:
	//Overview
	NNDocument *network_overview_;
	int network_overview_structure_index_;//-1 : Config
	NNGraphMapper result_graph_mapper_;
	int network_overview_update_count_;
	double network_overview_zoom_;
	double network_overview_current_zoom_;
	float network_overview_last_x_;
	float network_overview_last_y_;
	FString last_config_text_;
private:
	//Progress
	int progress_training_max_;
	int progress_training_current_;
	int progress_evaluation_max_;
	int progress_evaluation_current_;
private:
	//State
	int GetExecuting() const;
	int GetTraining(int local_only = false) const;
	int GetTraining(const FString &job_name) const;
	int GetEvaluating() const;
	int GetEvaluating(const FString &job_name) const;
public:
	//Common GUI
	void CheckSplashWindow();
	void SetEnvironment();//Set environment variable (PATH, PYTHON PATH)
	void WaitForDocumentUnlock();
	void RefreshRunMenu();

	void ShowAnimationIndicator(UnicodeString text = "", int show_log = false, int show_abort_button = false, int show_close_button = false, int auto_close = true, int hide_log = false);
	void AnimationIndicatorLog(UnicodeString log);
	void CheckAnimationIndicator();
	void HideAnimationIndicator();
	int animation_indicator_active_;
	unsigned int animation_indicator_start_time_;
	int animation_indicator_abort_;
	int animation_indicator_hide_log_;

	int closed_;

private:
	//Structure Search
	double structure_search_last_gaussian_process_beta_;
	double structure_search_last_gaussian_process_gaussian_kernel_sigma_;

	FListT<double> structure_search_candidate_myu_;
	FListT<double> structure_search_candidate_sigma_;
	FListT<double> structure_search_candidate_cost_;
	FListT<double> structure_search_candidate_gain_;
	int structure_search_candidate_index_;

	void ContinueStructureSearch();
	void MutateNetworkForStructureSearch_Random(const FListP<NNStructures> &result_structures, const FListT<int> &pareto_optimal_index_all_sorted, const FListT<int> &pareto_optimal_index_seed);
	int MutateNetworkForStructureSearch_NetworkFeatureAndGaussianProcess(const FListP<NNStructures> &result_structures, const FListT<int> &pareto_optimal_index_all_sorted, const FListT<int> &pareto_optimal_index_seed);
private:
	//Plugins
	NNCPlugins plugins_;
    FStringList plugin_history_;

	void RefreshPlugins();
	void RefreshPluginMenus(int all);
	void CreatePluginMenu(TMenuItem *menu_parent, FString plugin_dir, int include_history=false);
	void __fastcall PluginClick(TObject *Sender);
	int ExecutePlugin(const NNCPlugin &plugin);
	int ShowPluginWindow(const NNCPlugin& plugin, FString &args, FString *dir, FString *output_file_name, FStringList *dataset_files);
	void InstallPackages();
	void OpenEvaluationOthers(const FString &file_name);
	int OpenFileAsTable(const FString &file_name);
private:
	//Debug
	int online_debug_mode_;
	int DebugMessage(UnicodeString s, int test_pass = -1);

	FListT<int> command_;//for hidden command

	int screen_cursor_on_;//for screen capture
	void CheckLogLevel();
	int Log(FString args, int force_send = false);
	int LogTraceback(TStrings *str);
	void ClearDebugBMP() {if (debug_bmp_) {delete debug_bmp_; debug_bmp_ = NULL;}};
	TBitmap *debug_bmp_;

	int TestCheckIfAllLayerHasInitialWeight();
private:
	//Others

    int setup_request_;

	void ProcessMessages();
	void CheckDatasetRequirement();
	void PrepareRightSubPane();

	//translation
	UnicodeString default_lang_;
	FListT<TMenuItem*> menu_items_;
	FStringList menu_item_original_texts_;
	void ListMenuItemForTranslation(TComponent* parent);
	void ListMenuItemRecursiveForTranslation(TComponent* parent);
	void Translate();
	UnicodeString GetTranslatedText(UnicodeString text);
	FString GetTranslatedFString(UnicodeString text);
public:
	void SetScale(int gui_scale);
	int SetClipboardText(const FString &data);
	void SetActiveTab(TTabControl *TC, TTabItem *TI);
	void OpenManual();
};
//---------------------------------------------------------------------------
extern PACKAGE TFo_Main *Fo_Main;
//---------------------------------------------------------------------------
#endif
