// Part of BNC, a utility for retrieving decoding and
// converting GNSS data streams from NTRIP broadcasters.
//
// Copyright (C) 2007
// German Federal Agency for Cartography and Geodesy (BKG)
// http://www.bkg.bund.de
// Czech Technical University Prague, Department of Geodesy
// http://www.fsv.cvut.cz
//
// Email: euref-ip@bkg.bund.de
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, version 2.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

/* -------------------------------------------------------------------------
 * BKG NTRIP Client
 * -------------------------------------------------------------------------
 *
 * Class:      main
 *
 * Purpose:    Application starts here
 *
 * Author:     L. Mervart
 *
 * Created:    24-Dec-2005
 *
 * Changes:
 *
 * -----------------------------------------------------------------------*/

#ifndef WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif
#include <signal.h>
#include <QApplication>
#include <QFile>
#include <iostream>
#include <QNetworkProxyFactory>

#include "app.h"
#include "bnccore.h"
#include "bncwindow.h"
#include "bncsettings.h"
#include "bncversion.h"
#include "upload/bncephuploadcaster.h"
#include "rinex/reqcedit.h"
#include "rinex/reqcanalyze.h"
#include "orbComp/sp3Comp.h"

using namespace std;


void catch_SIGINT(int) {
  cout << "BNC Interrupted by Ctrl-C" << endl;
  BNC_CORE->sigintReceived = 1;
  BNC_CORE->stopPPP();
  BNC_CORE->stopCombination();
#ifndef WIN32
  sleep(2);
#else
  Sleep(2000);
#endif
  ::exit(0);
}

void catch_SIGTERM(int) {
  cout << "BNC Terminated" << endl;
  BNC_CORE->sigintReceived = 1;
  BNC_CORE->stopPPP();
  BNC_CORE->stopCombination();
#ifndef WIN32
  sleep(2);
#else
  Sleep(2000);
#endif
  ::exit(0);
}

void catch_SIGHUP(int) {
  cout << "BNC received SIGHUP signal: reload configuration" << endl;
  bncSettings settings;
  settings.reRead();
  BNC_CORE->caster()->readMountPoints();
}

// Main Program
/////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {

  bool       interactive  = true;
#ifdef WIN32
  bool       displaySet   = true;
#else
  bool       displaySet   = false;
#endif
  QByteArray          rawFileName;
  QString             confFileName;

  bncWindow*          bncWin = 0;
  t_reqcEdit*         reqcEdit = 0;
  t_reqcAnalyze*      reqcAnalyze = 0;
  t_sp3Comp*          sp3Comp = 0;
  bncEphUploadCaster* casterEph = 0;
  bncCaster*          caster = 0;
  bncRawFile*         rawFile =  0;
  bncGetThread*       getThread = 0;

  QByteArray printHelp =
      "Usage:\n"
      "   bnc --help (MS Windows: bnc.exe --help | more)\n"
      "       --nw\n"
      "       --version (MS Windows: bnc.exe --version | more)\n"
      "       --display {name}\n"
      "       --conf {confFileName}\n"
      "       --file {rawFileName}\n"
      "       --key  {keyName} {keyValue}\n"
      "\n"
      "Network Panel keys:\n"
      "   proxyHost         {Proxy host, name or IP address [character string]}\n"
      "   proxyPort         {Proxy port [integer number]}\n"
      "   sslCaCertPath     {Full path to SSL certificates [character string]}\n"
      "   sslClientCertPath {Full path to client SSL certificates [character string]}\n"
      "   sslIgnoreErrors   {Ignore SSL authorization errors [integer number: 0=no,2=yes]}\n"
      "\n"
      "General Panel keys:\n"
      "   logFile          {Logfile, full path [character string]}\n"
      "   rnxAppend        {Append files [integer number: 0=no,2=yes]}\n"
      "   onTheFlyInterval {Configuration reload interval [character string: no|1 day|1 hour|5 min|1 min]}\n"
      "   autoStart        {Auto start [integer number: 0=no,2=yes]}\n"
      "   rawOutFile       {Raw output file, full path [character string]}\n"
      "\n"
      "RINEX Observations Panel keys:\n"
      "   rnxPath        {Directory for RINEX files [character string]}\n"
      "   rnxIntr        {File interval [character string: 1 min|2 min|5 min|10 min|15 min|30 min|1 hour|1 day]}\n"
      "   rnxSampl       {File sampling rate [character string: 0.1 sec|1 sec|5 sec|10 sec|15 sec|30 sec|60 sec]}\n"
      "   rnxSkel        {RINEX skeleton file extension [character string: skl|SKL]}\n"
      "   rnxSkelPath    {Directory for local skeleton files [character string]}\n"
      "   rnxOnlyWithSKL {Using RINEX skeleton file is mandatory [integer number: 0=no,2=yes]}\n"
      "   rnxScript      {File upload script, full path [character string]}\n"
	  "   rnxVersion     {Specifies the RINEX version of the file contents [integer number: 2|3|4]}\n"
      "   rnxV2Priority  {Priority of signal attributes for RINEX version 2 [character string, list separated by blank character,\n"
      "                   example: G:12&PWCSLX G:5&IQX R:12&PC R:3&IQX R:46&ABX E:16&BCXZ E:578&IQX J:1&SLXCZ J:26&SLX J:5&IQX C:267&IQX C:18&DPX I:ABCX S:1&C S:5&IQX]}\n"
      "\n"
      "RINEX Ephemeris Panel keys:\n"
      "   ephPath        {Directory [character string]}\n"
      "   ephIntr        {File interval [character string: 1 min|2 min|5 min|10 min|15 min|30 min|1 hour|1 day]}\n"
      "   ephOutPort     {Output port [integer number]}\n"
      "   ephVersion     {Specifies the RINEX version of the file contents [integer number: 2|3|4]}\n"
      "\n"
      "RINEX Editing and QC Panel keys:\n"
      "   reqcAction            {Action specification [character string:  Blank|Edit/Concatenate|Analyze]}\n"
      "   reqcObsFile           {Input observations file(s), full path [character string, comma separated list in quotation marks]}\n"
      "   reqcNavFile           {Input navigation file(s), full path [character string, comma separated list in quotation marks]}\n"
      "   reqcOutObsFile        {Output observations file, full path [character string]}\n"
      "   reqcOutNavFile        {Output navigation file, full path [character string]}\n"
      "   reqcOutLogFile        {Output logfile, full path [character string]}\n"
      "   reqcLogSummaryOnly    {Output only summary of logfile [integer number: 0=no,2=yes]}\n"
      "   reqcSkyPlotSignals    {Observation signals [character string, list separated by blank character,\n"
      "                          example: G:1&2&5 R:1&2&3 E:1&7 C:2&6 J:1&2 I:5&9 S:1&5]}\n"
      "   reqcPlotDir           {QC plots directory [character string]}\n"
      "   reqcRnxVersion        {RINEX version [integer number: 2|3|4]}\n"
      "   reqcSampling          {RINEX output file sampling rate [character string: 0.1 sec|1 sec|5 sec|10 sec|15 sec|30 sec|60 sec]}\n"
      "   reqcV2Priority        {Version 2 priority of signal attributes [character string, list separated by blank character,\n"
      "                          example: G:12&PWCSLX G:5&IQX R:12&PC R:3&IQX R:46&ABX E:16&BCXZ E:578&IQX J:1&SLXCZ J:26&SLX J:5&IQX C:267&IQX C:18&DPX I:ABCX S:1&C S:5&IQX]}\n"
      "   reqcStartDateTime     {Start time [character string, example: 1967-11-02T00:00:00]}\n"
      "   reqcEndDateTime       {Stop time [character string, example: 2099-01-01T00:00:00]}\n"
      "   reqcRunBy             {Operators name [character string]}\n"
      "   reqcUseObsTypes       {Use observation types [character string, list separated by blank character, example: G:C1C G:L1C R:C1C RC1P]}\n"
      "   reqcComment           {Additional comments [character string]}\n"
      "   reqcOldMarkerName     {Old marker name [character string]}\n"
      "   reqcNewMarkerName     {New marker name [character string]}\n"
      "   reqcOldAntennaName    {Old antenna name [character string]}\n"
      "   reqcNewAntennaName    {New antenna name [character string]}\n"
      "   reqcOldAntennaNumber  {Old antenna number [character string]}\n"
      "   reqcNewAntennaNumber  {New antenna number [character string]}\n"
      "   reqcOldAntennadN      {Old north eccentricity [character string]}\n"
      "   reqcNewAntennadN      {New north eccentricity [character string]}\n"
      "   reqcOldAntennadE      {Old east eccentricity [character string]}\n"
      "   reqcNewAntennadE      {New east eccentricity [character string]}\n"
      "   reqcOldAntennadU      {Old up eccentricity [character string]}\n"
      "   reqcNewAntennadU      {New up eccentricity [character string]}\n"
      "   reqcOldReceiverName   {Old receiver name [character string]}\n"
      "   reqcNewReceiverName   {New receiver name [character string]}\n"
      "   reqcOldReceiverNumber {Old receiver number [character string]}\n"
      "   reqcNewReceiverNumber {New receiver number [character string]}\n"
      "\n"
      "SP3 Comparison Panel keys:\n"
      "   sp3CompFile        {SP3 input files, full path [character string, comma separated list in quotation marks]}\n"
      "   sp3CompExclude     {Satellite exclusion list [character string, comma separated list in quotation marks, example: G04,G31,R]}\n"
      "   sp3CompOutLogFile  {Output logfile, full path [character string]}\n"
      "   sp3CompSummaryOnly {Output only summary of logfile [integer number: 0=no,2=yes]}\n"
      "\n"
      "Broadcast Corrections Panel keys:\n"
      "   corrPath {Directory for saving files in ASCII format [character string]}\n"
      "   corrIntr {File interval [character string: 1 min|2 min|5 min|10 min|15 min|30 min|1 hour|1 day]}\n"
      "   corrPort {Output port [integer number]}\n"
      "\n"
      "Feed Engine Panel keys:\n"
      "   outPort  {Output port, synchronized [integer number]}\n"
      "   outWait  {Wait for full observation epoch [integer number of seconds: 1-30]}\n"
      "   outSampl {Sampling rate [character string: 0.1 sec|1 sec|5 sec|10 sec|15 sec|30 sec|60 sec]}\n"
      "   outFile  {Output file, full path [character string]}\n"
      "   outUPort {Output port, unsynchronized [integer number]}\n"
      "\n"
      "Serial Output Panel:\n"
      "   serialMountPoint         {Mountpoint [character string]}\n"
      "   serialPortName           {Port name [character string]}\n"
      "   serialBaudRate           {Baud rate [integer number: 110|300|600|1200|2400|4800|9600|19200|38400|57600|115200]}\n"
      "   serialFlowControl        {Flow control [character string: OFF|XONXOFF|HARDWARE}\n"
      "   serialDataBits           {Data bits [integer number: 5|6|7|8]}\n"
      "   serialParity             {Parity [character string: NONE|ODD|EVEN|SPACE]}\n"
      "   serialStopBits           {Stop bits [integer number: 1|2]}\n"
      "   serialAutoNMEA           {NMEA specification [character string: no|Auto|Manual GPGGA|Manual GNGGA]}\n"
      "   serialFileNMEA           {NMEA filename, full path [character string]}\n"
      "   serialHeightNMEA         {Height [floating-point number]}\n"
      "   serialHeightNMEASampling {Sampling rate [integer number of seconds: 0|10|20|30|...|280|290|300]}\n"
      "\n"
      "Outages Panel keys:\n"
      "   adviseObsRate {Stream observation rate [character string: 0.1 Hz|0.2 Hz|0.5 Hz|1 Hz|5 Hz]} \n"
      "   adviseFail    {Failure threshold [integer number of minutes: 0-60]}\n"
      "   adviseReco    {Recovery threshold [integer number of minutes: 0-60]}\n"
      "   adviseScript  {Advisory script, full path [character string]}\n"
      "\n"
      "Miscellaneous Panel keys:\n"
      "   miscMount    {Mountpoint [character string]}\n"
      "   miscIntr     {Interval for logging latency [character string: Blank|2 sec|10 sec|1 min|5 min|15 min|1 hour|6 hours|1 day]}\n"
      "   miscScanRTCM {Scan for RTCM message numbers [integer number: 0=no,2=yes]}\n"
      "   miscPort     {Output port [integer number]}\n"
      "\n"
      "PPP Client Panel 1 keys:\n"
      "   PPP/dataSource  {Data source [character string: Blank|Real-Time Streams|RINEX Files]}\n"
      "   PPP/rinexObs    {RINEX observation file, full path [character string]}\n"
      "   PPP/rinexNav    {RINEX navigation file, full path [character string]}\n"
      "   PPP/corrMount   {Corrections mountpoint [character string]}\n"
      "   PPP/corrFile    {Corrections file, full path [character string]}\n"
      "   PPP/crdFile     {Coordinates file, full path [character string]}\n"
      "   PPP/logPath     {Directory for PPP log files [character string]}\n"
      "   PPP/antexFile   {ANTEX file, full path [character string]}\n"
#ifdef USE_PPP
      "   PPP/blqFile     {BLQ file, full path [character string]}\n"
      "   PPP/ionoMount   {VTEC mountpoint, [char string]}\n"
      "   PPP/ionoFile    {VTEC file, full path [char string]}\n"
#endif
      "   PPP/nmeaPath      {Directory for NMEA output files [character string]}\n"
      "   PPP/snxtroPath    {Directory for SINEX troposphere output files [character string]}\n"
      "   PPP/snxtroIntr    {SINEX troposphere file interval [character string: 1 min|2 min|5 min|10 min|15 min|30 min|1 hour|1 day]}\n"
      "   PPP/snxtroSampl   {SINEX troposphere file sampling rate [character string: 1 sec|5 sec|10 sec|30 sec|60 sec|300 sec]}\n"
      "   PPP/snxtroAc      {SINEX troposphere Analysis Center [3-char string]}\n"
      "   PPP/snxtroSolId   {SINEX troposphere solution ID [1-char]}\n"
      "   PPP/snxtroSolType {SINEX troposphere solution type, e.g. RTS, UNK, .. [3-char]}\n"
      "   PPP/snxtroCampId  {SINEX troposphere campaign ID, e.g. DEM, TST, OPS, .. [3-char]}\n"
      "\n"
      "PPP Client Panel 2 keys:\n"
      "   PPP/lcGPS        {Select the kind of linear combination from GPS code and/or phase data [character string:     Pi&Li|Pi|P1&L1|P1|P3&L3|P3|L3|no]}\n"
      "   PPP/lcGLONASS    {Select the kind of linear combination from GLONASS code and/or phase data [character string: Pi&Li|Pi|P1&L1|P1|P3&L3|P3|L3|no]}\n"
      "   PPP/lcGalileo    {Select the kind of linear combination from Galileo code and/or phase data [character string: Pi&Li|Pi|P1&L1|P1|P3&L3|P3|L3|no]}\n"
      "   PPP/lcBDS        {Select the kind of linear combination from BDS code and/or phase data [character string:     Pi&Li|Pi|P1&L1|P1|P3&L3|P3|L3|no]}\n"
      "   PPP/sigmaC1      {Sigma for code observations in meters [floating-point number]}\n"
      "   PPP/sigmaL1      {Sigma for phase observations in meters [floating-point number]}\n"
      "   PPP/maxResC1     {Maximal residuum for code observations in meters [floating-point number]}\n"
      "   PPP/maxResL1     {Maximal residuum for phase observations in meters [floating-point number]}\n"
      "   PPP/eleWgtCode   {Elevation dependent waiting of code observations [integer number: 0=no,2=yes]}\n"
      "   PPP/eleWgtPhase  {Elevation dependent waiting of phase observations [integer number: 0=no,2=yes]}\n"
      "   PPP/minObs       {Minimum number of observations [integer number: 4|5|6]}\n"
      "   PPP/minEle       {Minimum satellite elevation in degrees [integer number: 0-20]}\n"
      "   PPP/corrWaitTime {Wait for clock corrections [integer number of seconds: 0-20]}\n"
      "   PPP/seedingTime  {Seeding time span for Quick Start [integer number of seconds]}\n"
#ifdef USE_PPP
      "   PPP/constraints  {Specify, whether ionospheric constraints in form of pseudo-observations shall be added [character string: no|Ionosphere: pseudo-obs]}\n"
      "   PPP/sigmaGIM     {Sigma for GIM pseudo observations in meters [floating-point number]}\n"
      "   PPP/maxResGIM    {Maximal residuum for GIM pseudo observations in meters [floating-point number]}\n"
#endif
      "\n"
      "PPP Client Panel 3 keys:\n"
      "   PPP/staTable {Station specifications table [character string, semicolon separated list, each element in quotaion marks, example:\n"
      "                \"WTZR00DEU0,100.0,100.0,100.0,100.0,100.0,100.0,0.1,3e-6,0,G:1&C G:2&W R:1&C R:2&P E:1&C E:5&Q C:26&I;\n"
      "                FFMJ00DEU0,100.0,100.0,100.0,100.0,100.0,100.0,0.1,3e-6,0,G:12&W R:12&P E:15&X C:26&I\"]}\n"
      "\n"
      "PPP Client Panel 4 keys:\n"
      "   PPP/plotCoordinates  {Mountpoint for time series plot [character string]}\n"
      "   PPP/audioResponse    {Audio response threshold in meters [floating-point number]}\n"
      "   PPP/mapWinDotSize    {Size of dots on map [integer number: 0-10]}\n"
      "   PPP/mapWinDotColor   {Color of dots and cross hair on map [character string: red|yellow]}\n"
      "   PPP/mapSpeedSlider   {Off-line processing speed for mapping [integer number: 1-100]}\n"
      "\n"
      "Combine Corrections Panel keys:\n"
      "   cmbStreams         {Correction streams table [character string, semicolon separated list, each element in quotation marks, example:\n"
      "                       \"SSRA00ESA0 ESA 1.0;SSRA00BKG BKG 1.0\"]}\n"
      "   cmbMethodFilter    {Combination approach [character string: Single-Epoch|Filter]}\n"
      "   cmbBsxFile         {SINEX Bias file, full path [char string]}\n"
      "   cmbMaxres          {Clock outlier residuum threshold in meters [floating-point number]}\n"
      "   cmbMaxdisplacement {Maximal orbit displacement from the mean of corrections for a satellite [floating-point number]}\n"
      "   cmbSampl           {Clock sampling rate [integer number of seconds: 0|10|20|30|40|50|60]}\n"
      "   cmbLogpath         {Directory for Combination log files [character string]}\n"
      "   cmbGps             {GPS correction usage [integer number: 0=no,2=yes]}\n"
      "   cmbGlo             {GLONASS correction usage [integer number: 0=no,2=yes]}\n"
      "   cmbGal             {Galileo correction usage [integer number: 0=no,2=yes]}\n"
      "   cmbBds             {Beidou correction usage [integer number: 0=no,2=yes]}\n"
      "   cmbQzss            {QZSS correction usage [integer number: 0=no,2=yes]}\n"
      "   cmbSbas            {SBAS correction usage [integer number: 0=no,2=yes]}\n"
      "   cmbNavic           {NavIC correction usage [integer number: 0=no,2=yes]}\n"
      "\n"
      "Upload Corrections Panel keys:\n"
      "   uploadMountpointsOut   {Upload corrections table [character string, semicolon separated list, each element in quotation marks, example:\n"
      "                          \"products.igs-ip.net,2101,SSRA02IGS1,2,usr,pass,IGS20,IGS-SSR,0,/home/user/BKG0MGXRTS${V3PROD}.SP3,/home/user/BKG0MGXRTS${V3PROD}.CLK,/home/user/BKG0MGXRTS${V3PROD}.BIA,258,1,0;\n"
      "                          euref-ip.net,2101,SSRA02IGS1_EUREF,2,usr,pass,ETRF2000,RTCM-SSR,0,,,,258,2,0\"]}\n"
      "   uploadIntr             {Length of SP3, Clock RINEX and Bias SINEX file interval [character string: 1 min|2 min|5 min|10 min|15 min|30 min|1 hour|1 day]}\n"
      "   uploadSamplRtcmEphCorr {Orbit corrections stream sampling rate [integer number of seconds: 0|5|10|15|20|25|30|35|40|45|50|55|60]}\n"
      "   uploadSamplSp3         {SP3 file sampling rate [integer number of seconds: 0 sec|30 sec|60 sec|300 sec|900 sec]}\n"
      "   uploadSamplClkRnx      {Clock RINEX file sampling rate [integer number of seconds: 0|5|10|15|20|25|30|35|40|45|50|55|60]}\n"
	  "   uploadSamplBiaSnx     {SINEX Bias file sampling rate [integer number of seconds: 0|5|10|15|20|25|30|35|40|45|50|55|60]}\n"
      "\n"
      "Custom Trafo keys:\n"
      "   trafo_dx  {Translation X in meters [floating-point number]\n"
      "   trafo_dy  {Translation Y in meters [floating-point number]\n"
      "   trafo_dz  {Translation Z in meters [floating-point number]\n"
      "   trafo_dxr {Translation change X in meters per year [floating-point number]\n"
      "   trafo_dyr {Translation change Y in meters per year [floating-point number]\n"
      "   trafo_dzr {Translation change Z in meters per year [floating-point number]\n"
      "   trafo_ox  {Rotation X in arcsec [floating-point number]}\n"
      "   trafo_oy  {Rotation Y in arcsec [floating-point number]}\n"
      "   trafo_oz  {Rotation Z in arcsec [floating-point number]}\n"
      "   trafo_oxr {Rotation change X in arcsec per year [floating-point number]}\n"
      "   trafo_oyr {Rotation change Y in arcsec per year [floating-point number]}\n"
      "   trafo_ozr {Rotation change Z in arcsec per year [floating-point number]}\n"
      "   trafo_sc  {Scale [10^-9, floating-point number]}\n"
      "   trafo_scr {Scale change [10^-9 per year, floating-point number]}\n"
      "   trafo_t0  {Reference year [integer number]}\n"
      "\n"
      "Upload Ephemeris Panel keys:\n"
      "   uploadEphMountpointsOut {Upload corrections table [character string, semicolon separated list, each element in quotation marks, example:\n"
      "                           \"products.igs-ip.net,443,BCEP00BKG0,2s,usr,pass,ALL;products.igs-ip.net,443,BCEP01BKG0,2s,usr,pass,G\"]}\n"
      "   uploadSamplRtcmEph      {Stream upload sampling rate [integer number of seconds: 0|5|10|15|20|25|30|35|40|45|50|55|60]}\n"
      "\n"
      "Add Stream keys:\n"
      "   mountPoints   {Mountpoints [character string, semicolon separated list, example:\n"
      "                 \"//user:pass@igs-ip.net:2101/FFMJ00DEU0 RTCM_3.3 DEU 50.09 8.66 no 2;//user:pass@mgex.igs-ip.net:2101/CUT000AUS0 RTCM_3.0 ETH 9.03 38.74 no 2\"}\n"
      "   ntripVersion  {Ntrip Version [character string: 1|2|2s|R|U]}\n"
      "   casterUrlList {Visited Broadcasters [character string, comma separated list]}\n"
      "\n"
      "Appearance keys:\n"
      "   startTab  {Index of top panel to be presented at start time [integer number: 0-17]}\n"
      "   statusTab {Index of bottom panel to be presented at start time [integer number: 0-3]}\n"
      "   font      {Font specification [character string in quotation marks, example: \"Helvetica,14,-1,5,50,0,0,0,0,0\"]}\n"
      "\n"
      "Note:\n"
      "The syntax of some command line configuration options slightly differs from that\n"
      "used in configuration files: Configuration file options which contain one or more blank\n"
      "characters or contain a semicolon separated parameter list must be enclosed by quotation\n"
      "marks when specified on command line.\n"
      "\n"
      "Examples command lines:\n"
      "(1) /home/weber/bin/bnc\n"
      "(2) /Applications/bnc.app/Contents/MacOS/bnc\n"
      "(3) /home/userName/bin/bnc --conf /home/userName/MyConfigFile.bnc\n"
      "(4) bnc --conf /Users/userName/.config/BKG/BNC.bnc -nw\n"
      "(5) bnc --conf /dev/null --key startTab 4 --key reqcAction Edit/Concatenate"
      " --key reqcObsFile BRUX00BEL_S_20211251100_15M_01S_MO.rnx --key reqcOutObsFile BRUX00BEL_S_20211251100_15M_01S_MO_OUT.rnx"
      " --key reqcRnxVersion 2 --key reqcSampling \"30 sec\" --key reqcV2Priority \"G:12&PWCSLX G:5&IQX\"\n"
      "(6) bnc --conf /dev/null --key mountPoints \"//user:pass@mgex.igs-ip.net:2101/CUT000AUS0 RTCM_3.0 ETH 9.03 38.74 no 2;"
      "//user:pass@igs-ip.net:2101/FFMJ00DEU1 RTCM_3.1 DEU 50.09 8.66 no 2\"\n"
      "(7) bnc --conf /dev/null --key startTab 15 --key cmbStreams \"SSRA00BKG1 BKG 1.0;SSRA00CNE1 CNES 1.0\"\n"
      "(8) bnc --conf /dev/null --key startTab 16 --key uploadMountpointsOut \"products.igs-ip.net,2101,SSRC00BKG1,2,usr,pass,IGS20,RTCM-SSR,2,/Users/userName/BKG0MGXRTS${V3PROD}.SP3,,,33,3,2;"
      "euref-ip.net,443,SSRA00BKG1_EUREF,2s,usr,pass,ETRF2000,IGS-SSR,0,,,,33,5,5\"\n"
      "(9) bnc --conf /dev/null --key startTab 13 --key PPP/dataSource \"Real-Time Streams\" --key PPP/staTable \"FFMJ00DEU1,100.0,100.0,100.0,100.0,100.0,100.0,0.1,3e-6,7777,G:12&W R:12&P E:1&C E:5&Q C:26&I;"
      "CUT000AUS0,100.0,100.0,100.0,100.0,100.0,100.0,0.1,3e-6,7778,G:1&C G:2&W R:12&P E:15&X C:26&I\"\n";


  for (int ii = 1; ii < argc; ii++) {
    if (QRegExp("--?help").exactMatch(argv[ii])) {
      cout << printHelp.data();
      exit(0);
    }
    if (QRegExp("--?nw").exactMatch(argv[ii])) {
      interactive = false;
    }
    if (QRegExp("--?version").exactMatch(argv[ii])) {
      cout << BNCPGMNAME << endl;
      exit(0);
    }
    if (QRegExp("--?display").exactMatch(argv[ii])) {
      displaySet = true;
      strcpy(argv[ii], "-display"); // make it "-display" not "--display"
    }
    if (ii + 1 < argc) {
      if (QRegExp("--?conf").exactMatch(argv[ii])) {
        confFileName = QString(argv[ii+1]);
      }
      if (QRegExp("--?file").exactMatch(argv[ii])) {
        interactive = false;
        rawFileName = QByteArray(argv[ii+1]);
      }
    }
  }

#ifdef Q_OS_MAC
  if (argc== 3 && interactive) {
    confFileName = QString(argv[2]);
  }
#else
  if (argc == 2 && interactive) {
    confFileName = QString(argv[1]);
  }
#endif

#ifdef Q_OS_MACX
    if ( QSysInfo::MacintoshVersion > QSysInfo::MV_10_8 )
    {
        // fix Mac OS X 10.9 (mavericks) font issue
        // https://bugreports.qt-project.org/browse/QTBUG-32789
        QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
    }
#endif

  bool GUIenabled = interactive || displaySet;
  QScopedPointer<QCoreApplication> app(createApplication(argc, argv, GUIenabled));

  if (qobject_cast<QApplication *>(app.data())) { // start GUI version
    app->setApplicationName("BNC");
    app->setOrganizationName("BKG");
    app->setOrganizationDomain("www.bkg.bund.de");   //app->setWindowIcon(QPixmap(":ntrip-logo.png"));
    BNC_CORE->setGUIenabled(GUIenabled);
  }
  BNC_CORE->setConfFileName( confFileName );

  bncSettings settings;

  // Proxy Settings
  // --------------
  QString proxyHost = settings.value("proxyHost").toString();
  int     proxyPort = settings.value("proxyPort").toInt();
  if (!proxyHost.isEmpty()) {
    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::HttpProxy);
    proxy.setHostName(proxyHost);
    proxy.setPort(proxyPort);
    QNetworkProxy::setApplicationProxy(proxy);
  }
  else {
    QNetworkProxyFactory::setUseSystemConfiguration(true);
  }

  for (int ii = 1; ii < argc - 2; ii++) {
    if (QRegExp("--?key").exactMatch(argv[ii])) {
      QString key(argv[ii+1]);
      QString val(argv[ii+2]);
      if (val.indexOf(";") != -1) {
        settings.setValue(key, val.split(";", Qt::SkipEmptyParts));
      }
      else {
        settings.setValue(key, val);
      }
    }
  }

  // Interactive Mode - open the main window
  // -----------------------------------------
  if (interactive) {

    BNC_CORE->setMode(t_bncCore::interactive);

    QString fontString = settings.value("font").toString();
    if ( !fontString.isEmpty() ) {
      QFont newFont;
      if (newFont.fromString(fontString)) {
        QApplication::setFont(newFont);
      }
    }

    bncWin = new bncWindow();
    BNC_CORE->setMainWindow(bncWin);
    bncWin->show();
  }

  // Post-Processing PPP
  // -------------------
  else if (settings.value("PPP/dataSource").toString() == "RINEX Files") {
    caster = new bncCaster();
    BNC_CORE->setCaster(caster);
    BNC_CORE->setMode(t_bncCore::batchPostProcessing);
    BNC_CORE->startPPP();
  }

  // Post-Processing reqc edit
  // -------------------------
  else if (settings.value("reqcAction").toString() == "Edit/Concatenate") {
    BNC_CORE->setMode(t_bncCore::batchPostProcessing);
    reqcEdit = new t_reqcEdit(0);
    reqcEdit->start();
  }

  // Post-Processing reqc analyze
  // ----------------------------
  else if (settings.value("reqcAction").toString() == "Analyze") {
    BNC_CORE->setMode(t_bncCore::batchPostProcessing);
    reqcAnalyze = new t_reqcAnalyze(0);
    reqcAnalyze->start();
  }

  // SP3 Files Comparison
  // --------------------
  else if (!settings.value("sp3CompFile").toString().isEmpty()) {
    BNC_CORE->setMode(t_bncCore::batchPostProcessing);
    sp3Comp = new t_sp3Comp(0);
    sp3Comp->start();
  }

  // Non-Interactive (data gathering)
  // --------------------------------
  else {

    signal(SIGINT, catch_SIGINT);
    signal(SIGTERM, catch_SIGTERM);
    BNC_CORE->sigintReceived = 0;
#ifndef WIN32
    signal(SIGHUP, catch_SIGHUP);
#endif

    casterEph = new bncEphUploadCaster(); (void) casterEph;

    caster = new bncCaster();

    BNC_CORE->setCaster(caster);
    BNC_CORE->setPortEph(settings.value("ephOutPort").toInt());
    BNC_CORE->setPortCorr(settings.value("corrPort").toInt());
    BNC_CORE->initCombination();

    BNC_CORE->connect(caster, SIGNAL(getThreadsFinished()), app->instance(), SLOT(quit()));

    BNC_CORE->slotMessage("========== Start BNC v" BNCVERSION " (" BNC_OS ") ==========", true);


    // Normal case - data from Internet
    // --------------------------------
    if ( rawFileName.isEmpty() ) {
      BNC_CORE->setMode(t_bncCore::nonInteractive);
      BNC_CORE->startPPP();

      caster->readMountPoints();
      if (caster->numStations() == 0) {
        BNC_CORE->slotMessage("bncMain: number of caster stations: 0 => exit" , true);
        exit(3);
      }
    }

    // Special case - data from file
    // -----------------------------
    else {
      BNC_CORE->setMode(t_bncCore::batchPostProcessing);
      BNC_CORE->startPPP();

      rawFile   = new bncRawFile(rawFileName, "", bncRawFile::input);
      getThread = new bncGetThread(rawFile);
      caster->addGetThread(getThread, true);
    }

  }

  // Start the application
  // ---------------------
  app->exec();

  // End of application
  // ------------------
  if (interactive) {
    delete bncWin;
  }
  else {
    BNC_CORE->stopPPP();
    BNC_CORE->stopCombination();
#ifndef WIN32
  sleep(2);
#else
  Sleep(2000);
#endif
  }
  if (caster) {
    delete caster; caster = 0; BNC_CORE->setCaster(0);
  }
  if (casterEph) {
    delete casterEph; casterEph = 0;
  }
  if (rawFile) {
    delete rawFile;
  }

  return 0;
}
