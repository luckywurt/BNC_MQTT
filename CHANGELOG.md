# Changelog
## 2.13.2 (2025-03-21)
- FIXED: Bug in SBAS health check
- FIXED: BUG in reading the RTNET format [(#198)](https://software.rtcm-ntrip.org/ticket/198)
- FIXED: Bug in writing code biases into a SSR file
- FIXED: Bug in GLO ephemeris RTCM3 encoder
- FIXED: Bug in BDT methods
- FIXED: Obs types from skl file can be used now to write them into RINEX version 3 AND 4 observation files as configured
- FIXED: **Bug in IGS SSR Epoch Time (BDS and GLO)**, which is defined as follows: Full seconds since the beginning of the week of continuous time scale with no offset from GPS, Galileo, QZSS, SBAS, **UTC leap seconds from GLONASS, -14 s offset from BDS**
- ADDED: 24:00 epoch in orbit and clock product files
- ADDED: Nav Type considerred in all applications: GPS/QZSS/NavIC: LNAV, GLONASS: FDMA_M, Galileo: I/NAV, BDS: D1/D2, SBAS: SBASL1
- ADDED: Service CRS and RTCM CRS message encodung and decoding
- ADDED: Data field range checks within RTCM3 ephemeris decoders mainly regarding TOC and TOE
- ADDED: Decoder string 'ZERO2FILE': Using this, BNC allows to by-pass its decoders and directly save the input in daily log files
- CHANGED: Decoder string 'ZERO' means now that the raw data are forwarded only
- CHANGED: According to the Galileo OS SDD v1.3 SHS=2 now leads to a newly-defined "EOM" status that means that the satellite signal may be used for PNT [(#185)](https://software.rtcm-ntrip.org/ticket/185)
- CHANGED: Obsolete qt-class members are replaced
- CHNAGED: from common receiver clock + ISBs to system specific receiver clocks in PPP mode
- CHANGED: 'IRNSS' replaced by 'NavIC'

## 2.13.1 (2024-04-30)
- FIXED: Health status of Galileo satellites is now determined by the combination of HS, DVS and SISA [(#174)](https://software.rtcm-ntrip.org/ticket/174)
- FIXED: Bug regarding the reading of the RINEX Version 4 Navigation files
- FIXED: Bug regarding the flexibel usage of any port in secure mode
- ADDED: A check in PPP and combination mode if stored ephemerides are still valid [(#176)](https://software.rtcm-ntrip.org/ticket/176)
- ADDED: A SIGHUP signal to BNC will result into a reload of its configuration file [(#179)](https://software.rtcm-ntrip.org/ticket/179)
- ADDED: The possibility to exclude satellites or satellite systems for individual ACs from the combination.
- ADDED: The possibility to ignore orbit corrections from an indiviul AC that differ by more than 'Maximal Orb Displacement' meters from the average of all orbit corrections per satellite.
- CHANGED: hDOP and vDOP values are computed from the variances of the NEU components of the receiver position estimates
- CHANGED: Sampling interval for Manual NMEA can be used now in the Auto NMEA option as well [(#122)](https://software.rtcm-ntrip.org/ticket/122)


## 2.13.0 (2023-12-01)
- ADDED: **RINEX Version 4 Support** (in navigation files currently the EPH message type is considered only)
- ADDED: **PPP Client Upgrade**
- ADDED: Multi-GNSS-PPP using uncombined code and/or phase data of two frequencies
- ADDED: Multi-GNSS-PPP using uncombined code and/or phase data of one frequency
- ADDED: Ionospheric constraints in form of pseudo observations can be added
- ADDED: **Ntrip Version 2.0 Upload including TLS**
- ADDED: **Multi-GNSS Satellite Clock Combination**
- ADDED: Individual code biases will be considered before satellite clocks are combined
- ADDED: Satellite clock combination result includes code biases
- ADDED: Ephemerides upload for different systems in parallel
- ADDED: **Multi-GNSS SP3, Clock RINEX and SINEX Bias** support
- ADDED: QC multipath analysis for more than two signals
- ADDED: Transition from Qt4 to **Qt5**


## 2.12.19 (2022-04-04)
- CHANGED: Minimum combination sampling interval


## 2.12.18 (2021-09-28)
- FIXED: Bug in ephemeris check
- ADDED: Satellite antenna phase center correction
- CHANGED: Some OSM updates to force https usage and another crossOriginKeyword
- CHANGED: Signal usage in BDS PPP


## 2.12.17 (2021-04-20)
- FIXED: Bug in SSR GLONASS upload
- FIXED: Redundant output concerning unhealthy satellites
- ADDED: One more line with satellite health flags for the GLONASS navigation message as defined in RINEX v. 3.05


## 2.12.16 (2021-02-25)
- FIXED: Bug in IRNSS ephemeris encoding / decoding
- FIXED: Bug in RTCM3coDecoder regrading SSR format
- FIXED: Bug regarding consideration of incoming ephemerides
- FIXED: Bug regarding latency check
- FIXED: Relativistic effects are adapted with respect to the respective GNSS ICD
- FIXED: Bug regarding the generation of combined SSR orbit and clock messages
- FIXED: Bug within ephemeris check
- ADDED: Check to prevent the same eph data sets with different TOC values
- CHANGED: A priori coordinates within examples are updated
- CHANGED: Changes to prevent erroneous ephemeris data sets from usage in combination
- CHANGED: Small format adaptations regarding latency check
- CHANGED: Range of MSM messages enlarged to 1237
- CHANGED: PPP map now with OSM only


## 2.12.15 (2020-11-10)
- FIXED: Cleanup of the example configurations
- FIXED: Bug regarding long mountpoint names in latency check and latency plots
- FIXED: Bug in clock rinex header line
- ADDED: Ephemeris checks and related debugging output
- ADDED: Check if orbit and clock corrections are out of range
- CHANGED: Cleanup of the relativistic effects w.r.t. IGS-SSR

## 2.12.14 (2020-09-04)
- ADDED: Encoding and decoding of IGS-SSR messages
- ADDED: B2b/7D BDS in signal mapping for MSM
- CHANGED: IRNSS experimental in official ephemeris message number
- CHANGED: Lock Time Indicator is replaced by cycle slip counter in feed engine output

## 2.12.13 (2020-06-09)
- FIXED: QZSS fit Interval is specified as flag in RINEX 3.04
- CHANGED: BDS SSR IOD is changed from 24 into 8 bit
- CHANGED: BDS and QZSS SSR signal and tracking mode is adapted

## 2.12.12 (2020-01-21)
- ADDED: GPS and QZSS fit Interval in hours
- CHANGED: Epehmeris check

## 2.12.11 (2019-11-08)
- CHANGED:: Harmonization of RTCM3 Signal ID Mapping and RTCM SSR Signal and Tracking Mode Identifiers for BDS and QZSS

## 2.12.10 (2019-10-02)
- FIXED: Multiple message indicator in SSR messages
- FIXED: GLONASS message frame time written into RINEX files
- ADDED: IRNSS ephemeris support in RTCM3 Encoder
- ADDED: IRNSS MSM and ephemeris support in RTCM3 Decoder
- CHANGED: RTCM signal mapping IDs for GLONASS
- CHANGED: RTCM signal mapping IDs for BDS based on RTCM BDSWG proposal from 9/2019 as far as consistent with RINEX v 3.04
- CHANGED: Up to 64 BDS prn codes are supported now


## 2.12.9 (2019-05-20)
- FIXED: Method to read the RTNET data buffer
- FIXED: Typo in rtnet2ssr transition for ssr upload
- CHANGED: Number of possible phase biases for SSR upload are enlarged to 100
- CHANGED: Missing phase biases are added within rtnet2ssr transition for ssr upload


## 2.12.8 (2019-05-06)
- FIXED:   Bug with respect to GLONASS upload
- FIXED:   Bug in NMEA output
- CHANGED: Transformation parameters from ITRF2014 into DFREF91
- CHANGED: Transformation parameters from ITRF2014 into ETRF2000
- CHANGED: GLONASS ephemerides validity is now up to 2 hours.
- CHANGED: Check with respect to wrong observation epochs


## 2.12.7 (2019-04-03)
- FIXED: Bug in NMEA checksum
- FIXED: Bug in RINEX file concatenation
- FIXED: Bug in RTCM3 ephemeris message for QZSS
- FIXED: Bug in RTCM3 ephemeris message for BDS and SBAS
- FIXED: RINEX version 3 filenames for re-sampled files
- FIXED: Bug in reqc ephemeris check
- FIXED: Bug in RINEX file concatenation
- FIXED: Bug in ephemeris upload
- FIXED: Bug in ephemeris check
- FIXED: Bug in latency check
- FIXED: Galileo geocentric gravitational constant is corrected
- FIXED: Encoding/decoding of all missing parameters in MT 1020
- FIXED: Bug in RTCM3 MSM Decoder
- ADDED: Updates regarding RINEX Version 3.04
- ADDED: Lock time in seconds as an optional feed engine output
- ADDED: Two more polynomial coefficients of the SSR clock correction message
- ADDED: One more parameter to describe the SSR URA
- ADDED: Decoding of receiver descriptor in MT 1033
- ADDED: Satellite health check
- ADDED: IRNSS support in RINEX QC
- CHANGED: Parameters for transformation of orbit and clock corrections from ITRF 2014 into ETRF2000 and DREF91,
- CHANGED: No updated transformation parameters for NDA83 available, hence deleted
- CHANGED: Allow 10 Hz observation data processing and re-sampling
- CHANGED: SSR SBAS and BDS satellite IDs with respect to the proposal
- CHANGED: Transformation of orbit and clock corrections into ETRF2000, NDA83 or DREF91 is done temporarily via ITRF2008


## 2.12.6 (2017-09-26)
- FIXED:: GPS SSR Orbit IOD has to be GPS IODE, not IODC

## 2.12.5 (2017-08-30)
- CHANGED: RTCM message number for BDS is updated

## 2.12.4 (2017-04-10)
- CHANGED: SIRGAS2000 transformation parameters adjusted to IGb14
- CHANGED: Transformation parameters for ITRF2005 to GDA94 removed
- CHANGED: Transformation parameters for ITRF2008 to SIRGAS95 removed
- CHANGED: Transformation parameters for ITRF2014 to GDA2020 added

## 2.12.3 (2016-09-05)
- FIXED: Check regarding wrong observation epochs is done during latency check as well to prevent erroneous latencies
- FIXED: Map generation from sourcetable entry
- FIXED: Re-implementation of the well proven approach (from BNC version 2.11) on how to wait for clock corrections in PPP mode
- FIXED: Some NMEA components in PPP output: Time stamp is now UTC, hDop value instead pDop value
- FIXED: RINEX 2.11 ephemeris output for an unknown transmission time
- ADDED: Number of allowed SSR biases is enlarged,
 -ADDED: Some future GLONASS signal and tracking mode identifiers
- ADDED: Data source information as comment within RINEX navigation file header
- ADDED: Frequency specific signal priorities are for RINEX v3 to RINEX v2 conversion
- ADDED: Consideration of provider ID changes in SSR streams during PPP analysis
- CHANGED: Small adaptations in qwtpolar sources to allow a successful compilation of BNC on a Raspberry Pi
- CHANGED: The real satellite visibility is considered, if the expected observations are computed in RINEX QC


## 2.12.2 (2016-05-18)
- FIXED: Wrong RINEX v2 header line excluded
- ADDED: Expected observations in RINEX QC
- ADDED: Limits for spherical harmonics/degree order extended

## 2.12.1 (2016-04-21)
- FIXED: RINEX v2 file naming for observation files
- CHANGED: Release number is now part of BNC version

## 2.12.0 (2016-04-19)

