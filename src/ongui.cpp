/*
  ongui.cpp

  The simpler GUI event handlers.
*/

#include "main.h"
#include "lpcnet_freedv.h"

extern int g_mode;
extern bool  g_modal;

extern int   g_SquelchActive;
extern float g_SquelchLevel;
extern int   g_analog;
extern int   g_split;
extern int   g_tx;
extern int   g_State, g_prev_State;
extern FreeDVInterface freedvInterface;
extern bool g_queueResync;
extern short *g_error_hist, *g_error_histn;
extern int g_resyncs;
extern int g_Nc;
extern int g_txLevel;
extern wxConfigBase *pConfig;
extern bool endingTx;
extern int g_outfifo1_empty;

//-------------------------------------------------------------------------
// Forces redraw of main panels on window resize.
//-------------------------------------------------------------------------
void MainFrame::topFrame_OnSize( wxSizeEvent& event )
{
    m_auiNbookCtrl->Refresh();
    TopFrame::topFrame_OnSize(event);
}

//-------------------------------------------------------------------------
// OnExitClick()
//-------------------------------------------------------------------------
void MainFrame::OnExitClick(wxCommandEvent& event)
{
    OnExit(event);
}

//-------------------------------------------------------------------------
// OnToolsAudio()
//-------------------------------------------------------------------------
void MainFrame::OnToolsAudio(wxCommandEvent& event)
{
    wxUnusedVar(event);
    int rv = 0;
    AudioOptsDialog *dlg = new AudioOptsDialog(NULL);
    rv = dlg->ShowModal();
    if(rv == wxID_OK)
    {
        dlg->ExchangeData(EXCHANGE_DATA_OUT);
    }
    delete dlg;
}

//-------------------------------------------------------------------------
// OnToolsAudioUI()
//-------------------------------------------------------------------------
void MainFrame::OnToolsAudioUI(wxUpdateUIEvent& event)
{
    event.Enable(!m_RxRunning);
}

//-------------------------------------------------------------------------
// OnToolsFilter()
//-------------------------------------------------------------------------
void MainFrame::OnToolsFilter(wxCommandEvent& event)
{
    wxUnusedVar(event);
    
    if (m_filterDialog == nullptr)
    {
         m_filterDialog = new FilterDlg(NULL, m_RxRunning, &m_newMicInFilter, &m_newSpkOutFilter);
    }
    else
    {
        m_filterDialog->Iconize(false);
        m_filterDialog->SetFocus();
        m_filterDialog->Raise();
    }
    
    m_filterDialog->Show();
}

//-------------------------------------------------------------------------
// OnToolsOptions()
//-------------------------------------------------------------------------
void MainFrame::OnToolsOptions(wxCommandEvent& event)
{
    wxUnusedVar(event);
    g_modal = true;
    //fprintf(stderr,"g_modal: %d\n", g_modal);
    optionsDlg->ShowModal();
    
    // Show/hide frequency box based on PSK Reporter status.
    m_freqBox->Show(wxGetApp().m_psk_enable);
}

//-------------------------------------------------------------------------
// OnToolsOptionsUI()
//-------------------------------------------------------------------------
void MainFrame::OnToolsOptionsUI(wxUpdateUIEvent& event)
{
}

//-------------------------------------------------------------------------
// OnToolsComCfg()
//-------------------------------------------------------------------------
void MainFrame::OnToolsComCfg(wxCommandEvent& event)
{
    wxUnusedVar(event);

    ComPortsDlg *dlg = new ComPortsDlg(NULL);

    dlg->ShowModal();

    delete dlg;
}

//-------------------------------------------------------------------------
// OnToolsComCfgUI()
//-------------------------------------------------------------------------
void MainFrame::OnToolsComCfgUI(wxUpdateUIEvent& event)
{
    event.Enable(!m_RxRunning);
}

//-------------------------------------------------------------------------
// OnHelpCheckUpdates()
//-------------------------------------------------------------------------
void MainFrame::OnHelpCheckUpdates(wxCommandEvent& event)
{
    wxMessageBox("Got Click!", "OnHelpCheckUpdates", wxOK);
    event.Skip();
}

//-------------------------------------------------------------------------
// OnHelpCheckUpdatesUI()
//-------------------------------------------------------------------------
void MainFrame::OnHelpCheckUpdatesUI(wxUpdateUIEvent& event)
{
    event.Enable(false);
}

//-------------------------------------------------------------------------
// OnHelpManual()
//-------------------------------------------------------------------------
void MainFrame::OnHelpManual( wxCommandEvent& event )
{
    wxLaunchDefaultBrowser("https://github.com/drowe67/freedv-gui/blob/master/USER_MANUAL.pdf");
}

//-------------------------------------------------------------------------
//OnHelpAbout()
//-------------------------------------------------------------------------
void MainFrame::OnHelpAbout(wxCommandEvent& event)
{
    wxUnusedVar(event);
    wxString msg;
    msg.Printf( wxT("FreeDV GUI %s\n\n")
                wxT("For Help and Support visit: http://freedv.org\n\n")

                wxT("GNU Public License V2.1\n\n")
                wxT("Created by David Witten KD0EAG and David Rowe VK5DGR (2012).  ")
                wxT("Currently maintained by Mooneer Salem K6AQ and David Rowe VK5DGR.\n\n")
                wxT("freedv-gui version: %s\n")
                wxT("freedv-gui git hash: %s\n")
                wxT("codec2 git hash: %s\n")
                wxT("lpcnet git hash: %s\n"),
                FREEDV_VERSION, FREEDV_VERSION, GIT_HASH, freedv_get_hash(), lpcnet_get_hash());

    wxMessageBox(msg, wxT("About"), wxOK | wxICON_INFORMATION, this);
}


// Attempt to talk to rig using Hamlib

bool MainFrame::OpenHamlibRig() {
    if (wxGetApp().m_boolHamlibUseForPTT != true)
       return false;
    if (wxGetApp().m_intHamlibRig == 0)
        return false;
    if (wxGetApp().m_hamlib == NULL)
        return false;

    int rig = wxGetApp().m_intHamlibRig;
    wxString port = wxGetApp().m_strHamlibSerialPort;
    int serial_rate = wxGetApp().m_intHamlibSerialRate;
    bool status = wxGetApp().m_hamlib->connect(rig, port.mb_str(wxConvUTF8), serial_rate, wxGetApp().m_intHamlibIcomCIVHex);
    if (status == false)
    {
        wxMessageBox("Couldn't connect to Radio with hamlib", wxT("Error"), wxOK | wxICON_ERROR, this);
    }
    else
    {
        wxGetApp().m_hamlib->enable_mode_detection(m_txtModeStatus, m_txtCtrlReportFrequency, g_mode == FREEDV_MODE_2400B);
    }
    
    return status;
}

//-------------------------------------------------------------------------
// OnCloseFrame()
//-------------------------------------------------------------------------
void MainFrame::OnCloseFrame(wxCloseEvent& event)
{
    //fprintf(stderr, "MainFrame::OnCloseFrame()\n");
    auto engine = AudioEngineFactory::GetAudioEngine();
    engine->stop();
    engine->setOnEngineError(nullptr, nullptr);
    
    Destroy();
}

//-------------------------------------------------------------------------
// OnTop()
//-------------------------------------------------------------------------
void MainFrame::OnTop(wxCommandEvent& event)
{
    int style = GetWindowStyle();

    if (style & wxSTAY_ON_TOP)
    {
        style &= ~wxSTAY_ON_TOP;
    }
    else
    {
        style |= wxSTAY_ON_TOP;
    }
    SetWindowStyle(style);
}

//-------------------------------------------------------------------------
// OnDeleteConfig()
//-------------------------------------------------------------------------
void MainFrame::OnDeleteConfig(wxCommandEvent&)
{
    if(pConfig->DeleteAll())
    {
        wxLogMessage(wxT("Config file/registry key successfully deleted."));
        
        // Resets all configuration to defaults.
        loadConfiguration_();
    }
    else
    {
        wxLogError(wxT("Deleting config file/registry key failed."));
    }
}

//-------------------------------------------------------------------------
// OnDeleteConfigUI()
//-------------------------------------------------------------------------
void MainFrame::OnDeleteConfigUI( wxUpdateUIEvent& event )
{
    event.Enable(!m_RxRunning);
}

//-------------------------------------------------------------------------
// Paint()
//-------------------------------------------------------------------------
void MainFrame::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);

    if(GetMenuBar()->IsChecked(ID_PAINT_BG))
    {
        dc.Clear();
    }
    dc.SetUserScale(m_zoom, m_zoom);
}

//-------------------------------------------------------------------------
// OnCmdSliderScroll()
//-------------------------------------------------------------------------
void MainFrame::OnCmdSliderScroll(wxScrollEvent& event)
{
    char sqsnr[15];
    g_SquelchLevel = (float)m_sliderSQ->GetValue()/2.0 - 5.0;
    snprintf(sqsnr, 15, "%4.1f dB", g_SquelchLevel); // 0.5 dB steps
    wxString sqsnr_string(sqsnr);
    m_textSQ->SetLabel(sqsnr_string);

    event.Skip();
}

//-------------------------------------------------------------------------
// OnChangeTxLevel()
//-------------------------------------------------------------------------
void MainFrame::OnChangeTxLevel( wxScrollEvent& event )
{
    char fmt[15];
    g_txLevel = m_sliderTxLevel->GetValue();
    snprintf(fmt, 15, "%0.1f dB", (double)(g_txLevel)/10.0);
    wxString fmtString(fmt);
    m_txtTxLevelNum->SetLabel(fmtString);
    
    pConfig->Write(wxT("/Audio/transmitLevel"), g_txLevel);
}

//-------------------------------------------------------------------------
// OnCheckSQClick()
//-------------------------------------------------------------------------
void MainFrame::OnCheckSQClick(wxCommandEvent& event)
{
    if(!g_SquelchActive)
    {
        g_SquelchActive = true;
    }
    else
    {
        g_SquelchActive = false;
    }
}

void MainFrame::setsnrBeta(bool snrSlow)
{
    if(snrSlow)
    {
        m_snrBeta = 0.95; // make this closer to 1.0 to smooth SNR est further
    }
    else
    {
        m_snrBeta = 0.0; // no smoothing of SNR estimate from demodulator
    }
}

//-------------------------------------------------------------------------
// OnCheckSQClick()
//-------------------------------------------------------------------------
void MainFrame::OnCheckSNRClick(wxCommandEvent& event)
{
    wxGetApp().m_snrSlow = m_ckboxSNR->GetValue();
    setsnrBeta(wxGetApp().m_snrSlow);
    //printf("m_snrSlow: %d\n", (int)wxGetApp().m_snrSlow);
}

// check for space bar press (only when running)

int MainApp::FilterEvent(wxEvent& event)
{
    if ((event.GetEventType() == wxEVT_KEY_DOWN) &&
        (((wxKeyEvent&)event).GetKeyCode() == WXK_SPACE))
        {
            // only use space to toggle PTT if we are running and no modal dialogs (like options) up
            //fprintf(stderr,"frame->m_RxRunning: %d g_modal: %d\n",
            //        frame->m_RxRunning, g_modal);
            if (frame->m_RxRunning && !g_modal) {

                // space bar controls rx/rx if keyer not running
                if (frame->vk_state == VK_IDLE) {
                    if (frame->m_btnTogPTT->GetValue())
                        frame->m_btnTogPTT->SetValue(false);
                    else
                        frame->m_btnTogPTT->SetValue(true);

                    frame->togglePTT();
                }
                else // space bar stops keyer
                    frame->VoiceKeyerProcessEvent(VK_SPACE_BAR);

                return true; // absorb space so we don't toggle control with focus (e.g. Start)

            }
        }

    return -1;
}

//-------------------------------------------------------------------------
// OnTogBtnPTT ()
//-------------------------------------------------------------------------
void MainFrame::OnTogBtnPTT (wxCommandEvent& event)
{
    if (vk_state == VK_TX)
    {
        // Disable TX via VK code to prevent state inconsistencies.
        VoiceKeyerProcessEvent(VK_SPACE_BAR);
    }
    else
    {
        togglePTT();
    }
    event.Skip();
}

void MainFrame::togglePTT(void) {

    // Change tabbed page in centre panel depending on PTT state

    if (g_tx)
    {
        // Sleep for long enough that we get the remaining [blocksize] ms of audio.
        int msSleep = (1000 * freedvInterface.getTxNumSpeechSamples()) / freedvInterface.getTxSpeechSampleRate();
        if (g_verbose) fprintf(stderr, "Sleeping for %d ms prior to ending TX\n", msSleep);
        wxThread::Sleep(msSleep);
        
        // Trigger end of TX processing. This causes us to wait for the remaining samples
        // to flow through the system before toggling PTT.  Note 1000ms timeout as backup
        int sample = g_outfifo1_empty;
        endingTx = true;

        int i = 0;
        while ((i < 20) && (g_outfifo1_empty == sample)) {
            i++;
            wxThread::Sleep(50);
        }
        
        // tx-> rx transition, swap to the page we were on for last rx
        m_auiNbookCtrl->ChangeSelection(wxGetApp().m_rxNbookCtrl);

        // enable sync text

        m_textSync->Enable();
        m_textCurrentDecodeMode->Enable();
        
        // Re-enable On/Off button.
        m_togBtnOnOff->Enable(true);
    }
    else
    {
        // rx-> tx transition, swap to Mic In page to monitor speech
        wxGetApp().m_rxNbookCtrl = m_auiNbookCtrl->GetSelection();
        m_auiNbookCtrl->ChangeSelection(m_auiNbookCtrl->GetPageIndex((wxWindow *)m_panelSpeechIn));

        // disable sync text

        m_textSync->Disable();
        m_textCurrentDecodeMode->Disable();

        // Disable On/Off button.
        m_togBtnOnOff->Enable(false);
    }

    g_tx = m_btnTogPTT->GetValue();

    // Hamlib PTT

    if (wxGetApp().m_boolHamlibUseForPTT) {
        Hamlib *hamlib = wxGetApp().m_hamlib;
        wxString hamlibError;
        if (wxGetApp().m_boolHamlibUseForPTT && hamlib != NULL) {
            // Update mode display on the bottom of the main UI.
            if (hamlib->update_frequency_and_mode() != 0 || hamlib->ptt(g_tx, hamlibError) == false) {
                wxMessageBox(wxString("Hamlib PTT Error: ") + hamlibError, wxT("Error"), wxOK | wxICON_ERROR, this);
            }
        }
    }

    // Serial PTT

    if (wxGetApp().m_boolUseSerialPTT && (wxGetApp().m_serialport->isopen())) {
        wxGetApp().m_serialport->ptt(g_tx);
    }

    // reset level gauge

    m_maxLevel = 0;
    m_textLevel->SetLabel(wxT(""));
    m_gaugeLevel->SetValue(0);
    endingTx = false;
}


void MainFrame::OnTogBtnSplitClick(wxCommandEvent& event) {
    if (g_split)
        g_split = 0;
    else
        g_split = 1;
    event.Skip();
}

//-------------------------------------------------------------------------
// OnTogBtnAnalogClick()
//-------------------------------------------------------------------------
void MainFrame::OnTogBtnAnalogClick (wxCommandEvent& event)
{
    if (g_analog == 0) {
        g_analog = 1;
        m_panelSpectrum->setFreqScale(MODEM_STATS_NSPEC*((float)MAX_F_HZ/(FS/2)));
        m_panelWaterfall->setFs(FS);
    }
    else {
        g_analog = 0;
        m_panelSpectrum->setFreqScale(MODEM_STATS_NSPEC*((float)MAX_F_HZ/(freedvInterface.getRxModemSampleRate()/2)));
        m_panelWaterfall->setFs(freedvInterface.getRxModemSampleRate());
    }

    g_State = g_prev_State = 0;
    freedvInterface.getCurrentRxModemStats()->snr_est = 0;

    event.Skip();
}

void MainFrame::OnCallSignReset(wxCommandEvent& event)
{
    m_pcallsign = m_callsign;
    memset(m_callsign, 0, MAX_CALLSIGN);
    wxString s;
    s.Printf("%s", m_callsign);
    m_txtCtrlCallSign->SetValue(s);
}


// Force manual resync, just in case demod gets stuck on false sync

void MainFrame::OnReSync(wxCommandEvent& event)
{
    if (m_RxRunning)  {
        if (g_verbose) fprintf(stderr,"OnReSync\n");
        
        // Resync must be triggered from the TX/RX thread, so pushing the button queues it until
        // the next execution of the TX/RX loop.
        g_queueResync = true;
    }
}

void MainFrame::resetStats_()
{
    if (m_RxRunning)  {
        freedvInterface.resetBitStats();
        g_resyncs = 0;
        int i;
        for(i=0; i<2*g_Nc; i++) {
            g_error_hist[i] = 0;
            g_error_histn[i] = 0;
        }
        // resets variance stats every time it is called
        freedvInterface.setEq(wxGetApp().m_700C_EQ);
    }
}

void MainFrame::OnBerReset(wxCommandEvent& event)
{
    resetStats_();
}

void MainFrame::OnChangeReportFrequency( wxCommandEvent& event )
{
    wxString freqStr = m_txtCtrlReportFrequency->GetValue();
    if (freqStr.Length() > 0)
    {
        wxGetApp().m_psk_freq = atof(freqStr.ToUTF8()) * 1000;
        if (wxGetApp().m_psk_freq > 0)
        {
            m_txtCtrlReportFrequency->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
        }
        else
        {
            m_txtCtrlReportFrequency->SetForegroundColour(wxColor(*wxRED));
        }
    }
    else
    {
        wxGetApp().m_psk_freq = 0;
        m_txtCtrlReportFrequency->SetForegroundColour(wxColor(*wxRED));
    }
}

