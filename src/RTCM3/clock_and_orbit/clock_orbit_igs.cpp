/* Programheader

 Name:           clock_orbit_igs.c
 Project:        RTCM3
 Version:        $Id: clock_orbit_igs.c 8963 2020-06-29 12:46:25Z stuerze $
 Authors:        Dirk Stöcker, Andrea Stürze
 Description:    state space approach: IGS
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#ifndef sparc
#include <stdint.h>
#else
#include <sys/types.h>
#endif
#include <clock_orbit_igs.h>


size_t SsrCorrIgs::MakeClockOrbit(const struct ClockOrbit *co, ClockOrbitType type,
    int moremessagesfollow, char *buffer, size_t size) {
  std::vector< std::vector<unsigned int> > status(CLOCKORBIT_SATNUM, std::vector<unsigned int>(COBOFS_NUM));
  unsigned int i, s;

  STARTDATA

  for (s = 0; s < CLOCKORBIT_SATNUM; ++s) {
    for (i = 0; i < COBOFS_NUM; ++i) {
      if (co->NumberOfSat[s] && (type == COTYPE_AUTO || type == corbase[s] + i) &&
           (co->Supplied[i] || (i <= COBOFS_CLOCK &&  co->Supplied[COBOFS_COMBINED]) ||
           (i == COBOFS_COMBINED && co->Supplied[COBOFS_ORBIT] && co->Supplied[COBOFS_CLOCK]))) {
        status[s][i] = 1;
        if (i == COBOFS_COMBINED) {
          status[s][COBOFS_ORBIT] = status[s][COBOFS_CLOCK] = 0;
        } /* disable single blocks for combined type */
      } /* check for data */
    } /* iterate over sub-types */
  } /* iterate over satellite systems */

  for (s = 0; s < CLOCKORBIT_SATNUM; ++s) {
    if (status[s][COBOFS_ORBIT]) {
      INITBLOCK
	    T_RTCM_MESSAGE_NUMBER(RTCM_MESSAGE_NUMBER_IGS)
      T_IGS_SSR_VERSION(IGS_SSR_VERSION)
	    T_IGS_MESSAGE_NUMBER(corbase[s] + COBOFS_ORBIT)
	    T_SSR_EPOCH_TIME(co->EpochTime[s])
      T_SSR_UPDATE_INTERVAL(co->UpdateInterval)
	    T_MULTIPLE_MESSAGE_INDICATOR(moremessagesfollow ? 1 : 0)
      T_SSR_IOD(co->SSRIOD)
      T_SSR_PROVIDER_ID(co->SSRProviderID)
      T_SSR_SOLUTION_ID(co->SSRSolutionID)
	    T_SATELLITE_REFERENCE_DATUM(co->SatRefDatum)
      T_NO_OF_SATELLITES(co->NumberOfSat[s])
      for (i = satoffset[s]; i < satoffset[s] + co->NumberOfSat[s]; ++i) {
        T_GNSS_SATELLITE_ID(co->Sat[i].ID)
        T_GNSS_IOD(co->Sat[i].IOD)
        T_DELTA_RADIAL(co->Sat[i].Orbit.DeltaRadial)
        T_DELTA_ALONG_TRACK(co->Sat[i].Orbit.DeltaAlongTrack)
        T_DELTA_CROSS_TRACK(co->Sat[i].Orbit.DeltaCrossTrack)
        T_DELTA_DOT_RADIAL(co->Sat[i].Orbit.DotDeltaRadial)
        T_DELTA_DOT_ALONG_TRACK(co->Sat[i].Orbit.DotDeltaAlongTrack)
        T_DELTA_DOT_CROSS_TRACK(co->Sat[i].Orbit.DotDeltaCrossTrack)
      }
      ENDBLOCK
    }
    if (status[s][COBOFS_CLOCK]) {
      INITBLOCK
	    T_RTCM_MESSAGE_NUMBER(RTCM_MESSAGE_NUMBER_IGS)
      T_IGS_SSR_VERSION(IGS_SSR_VERSION)
	    T_IGS_MESSAGE_NUMBER(corbase[s] + COBOFS_CLOCK)
	    T_SSR_EPOCH_TIME(co->EpochTime[s])
      T_SSR_UPDATE_INTERVAL(co->UpdateInterval)
	    T_MULTIPLE_MESSAGE_INDICATOR(moremessagesfollow ? 1 : 0)
      T_SSR_IOD(co->SSRIOD)
      T_SSR_PROVIDER_ID(co->SSRProviderID)
      T_SSR_SOLUTION_ID(co->SSRSolutionID)
      T_NO_OF_SATELLITES(co->NumberOfSat[s])
      for (i = satoffset[s]; i < satoffset[s] + co->NumberOfSat[s]; ++i) {
        T_GNSS_SATELLITE_ID(co->Sat[i].ID)
        T_DELTA_CLOCK_C0(co->Sat[i].Clock.DeltaA0)
        T_DELTA_CLOCK_C1(co->Sat[i].Clock.DeltaA1)
        T_DELTA_CLOCK_C2(co->Sat[i].Clock.DeltaA2)
      }
      ENDBLOCK
    }
    if (status[s][COBOFS_COMBINED]) {
#ifdef SPLITBLOCK
      unsigned int nums = co->NumberOfSat[s];
      unsigned int left, start = satoffset[s];
      if(nums > 28) {/* split block when more than 28 sats */
        left = nums - 28;
        nums = 28;
      }
      else {
        left = 0;
      }
      while(nums) {
#endif
      INITBLOCK
	    T_RTCM_MESSAGE_NUMBER(RTCM_MESSAGE_NUMBER_IGS)
      T_IGS_SSR_VERSION(IGS_SSR_VERSION)
	    T_IGS_MESSAGE_NUMBER(corbase[s] + COBOFS_COMBINED)
	    T_SSR_EPOCH_TIME(co->EpochTime[s])
      T_SSR_UPDATE_INTERVAL(co->UpdateInterval)
#ifdef SPLITBLOCK
      T_MULTIPLE_MESSAGE_INDICATOR((moremessagesfollow || left) ? 1 : 0)
#else
      T_MULTIPLE_MESSAGE_INDICATOR(moremessagesfollow ? 1 : 0)
#endif
      T_SSR_IOD(co->SSRIOD)
      T_SSR_PROVIDER_ID(co->SSRProviderID)
      T_SSR_SOLUTION_ID(co->SSRSolutionID)
	  T_SATELLITE_REFERENCE_DATUM(co->SatRefDatum)
#ifdef SPLITBLOCK
      T_NO_OF_SATELLITES(nums)
      for(i = start; i < start+nums; ++i) {
#else
      T_NO_OF_SATELLITES(co->NumberOfSat[s])
      for (i = satoffset[s]; i < satoffset[s] + co->NumberOfSat[s]; ++i) {
#endif
        T_GNSS_SATELLITE_ID(co->Sat[i].ID)
        T_GNSS_IOD(co->Sat[i].IOD)
        T_DELTA_RADIAL(co->Sat[i].Orbit.DeltaRadial)
        T_DELTA_ALONG_TRACK(co->Sat[i].Orbit.DeltaAlongTrack)
        T_DELTA_CROSS_TRACK(co->Sat[i].Orbit.DeltaCrossTrack)
        T_DELTA_DOT_RADIAL(co->Sat[i].Orbit.DotDeltaRadial)
        T_DELTA_DOT_ALONG_TRACK(co->Sat[i].Orbit.DotDeltaAlongTrack)
        T_DELTA_DOT_CROSS_TRACK(co->Sat[i].Orbit.DotDeltaCrossTrack)
        T_DELTA_CLOCK_C0(co->Sat[i].Clock.DeltaA0)
        T_DELTA_CLOCK_C1(co->Sat[i].Clock.DeltaA1)
        T_DELTA_CLOCK_C2(co->Sat[i].Clock.DeltaA2)
      }
      ENDBLOCK
#ifdef SPLITBLOCK
      start += nums;
      nums = left;
      left = 0;
    }
#endif
    }
    if (status[s][COBOFS_HR]) {
      INITBLOCK
	    T_RTCM_MESSAGE_NUMBER(RTCM_MESSAGE_NUMBER_IGS)
      T_IGS_SSR_VERSION(IGS_SSR_VERSION)
	    T_IGS_MESSAGE_NUMBER(corbase[s] + COBOFS_HR)
	    T_SSR_EPOCH_TIME(co->EpochTime[s])
      T_SSR_UPDATE_INTERVAL(co->UpdateInterval)
	    T_MULTIPLE_MESSAGE_INDICATOR(moremessagesfollow ? 1 : 0)
      T_SSR_IOD(co->SSRIOD)
      T_SSR_PROVIDER_ID(co->SSRProviderID)
      T_SSR_SOLUTION_ID(co->SSRSolutionID)
      T_NO_OF_SATELLITES(co->NumberOfSat[s])
      for (i = satoffset[s]; i < satoffset[s] + co->NumberOfSat[s]; ++i) {
        T_GNSS_SATELLITE_ID(co->Sat[i].ID)
        T_HR_CLOCK_CORRECTION(co->Sat[i].hrclock)
      }
      ENDBLOCK
    }
    if (status[s][COBOFS_URA]) {
      INITBLOCK
	    T_RTCM_MESSAGE_NUMBER(RTCM_MESSAGE_NUMBER_IGS)
      T_IGS_SSR_VERSION(IGS_SSR_VERSION)
	    T_IGS_MESSAGE_NUMBER(corbase[s] + COBOFS_URA)
	    T_SSR_EPOCH_TIME(co->EpochTime[s])
      T_SSR_UPDATE_INTERVAL(co->UpdateInterval)
	    T_MULTIPLE_MESSAGE_INDICATOR(moremessagesfollow ? 1 : 0)
      T_SSR_IOD(co->SSRIOD)
      T_SSR_PROVIDER_ID(co->SSRProviderID)
      T_SSR_SOLUTION_ID(co->SSRSolutionID)
      T_NO_OF_SATELLITES(co->NumberOfSat[s])
      for (i = satoffset[s]; i < satoffset[s] + co->NumberOfSat[s]; ++i) {
        T_GNSS_SATELLITE_ID(co->Sat[i].ID)
        T_SSR_URA(ValueToURA(co->Sat[i].UserRangeAccuracy))
      }
      ENDBLOCK
    }
  }
  return ressize;
}

size_t SsrCorrIgs::MakeCodeBias(const struct CodeBias *b, CodeBiasType type,
    int moremessagesfollow, char *buffer, size_t size) {
  unsigned int s, i, j;

  STARTDATA

  for (s = 0; s < CLOCKORBIT_SATNUM; ++s) {
    if (b->NumberOfSat[s] && (type == CBTYPE_AUTO || type == corbase[s] + COBOFS_CBIAS)) {
      INITBLOCK
	    T_RTCM_MESSAGE_NUMBER(RTCM_MESSAGE_NUMBER_IGS)
      T_IGS_SSR_VERSION(IGS_SSR_VERSION)
	    T_IGS_MESSAGE_NUMBER(corbase[s] + COBOFS_CBIAS)
	    T_SSR_EPOCH_TIME(b->EpochTime[s])
      T_SSR_UPDATE_INTERVAL(b->UpdateInterval)
	    T_MULTIPLE_MESSAGE_INDICATOR(moremessagesfollow ? 1 : 0)
      T_SSR_IOD(b->SSRIOD)
      T_SSR_PROVIDER_ID(b->SSRProviderID)
      T_SSR_SOLUTION_ID(b->SSRSolutionID)
      T_NO_OF_SATELLITES(b->NumberOfSat[s])
      for (i = satoffset[s]; i < satoffset[s] + b->NumberOfSat[s]; ++i) {
        T_GNSS_SATELLITE_ID(b->Sat[i].ID)
        T_NO_OF_BIASES(b->Sat[i].NumberOfCodeBiases)
        for (j = 0; j < b->Sat[i].NumberOfCodeBiases; ++j) {
          T_GNSS_SIGNAL_IDENTIFIER(b->Sat[i].Biases[j].Type)
          T_CODE_BIAS(b->Sat[i].Biases[j].Bias)
        }
      }
      ENDBLOCK
    }
  }
  return ressize;
}

size_t SsrCorrIgs::MakePhaseBias(const struct PhaseBias *b, PhaseBiasType type,
    int moremessagesfollow, char *buffer, size_t size) {
  unsigned int s, i, j;

  STARTDATA

  for (s = 0; s < CLOCKORBIT_SATNUM; ++s)       {
    if (b->NumberOfSat[s] && (type == PBTYPE_AUTO || type == corbase[s] + COBOFS_PBIAS)) {
      INITBLOCK
	    T_RTCM_MESSAGE_NUMBER(RTCM_MESSAGE_NUMBER_IGS)
      T_IGS_SSR_VERSION(IGS_SSR_VERSION)
	    T_IGS_MESSAGE_NUMBER(corbase[s] + COBOFS_PBIAS)
	    T_SSR_EPOCH_TIME(b->EpochTime[s])
      T_SSR_UPDATE_INTERVAL(b->UpdateInterval)
	    T_MULTIPLE_MESSAGE_INDICATOR(moremessagesfollow ? 1 : 0)
      T_SSR_IOD(b->SSRIOD)
      T_SSR_PROVIDER_ID(b->SSRProviderID)
      T_SSR_SOLUTION_ID(b->SSRSolutionID)
      T_DISPERSIVE_BIAS_INDICATOR(b->DispersiveBiasConsistencyIndicator ? 1 : 0)
      T_MW_CONSISTENCY_INDICATOR(b->MWConsistencyIndicator ? 1 : 0)
      T_NO_OF_SATELLITES(b->NumberOfSat[s])
      for (i = satoffset[s]; i < satoffset[s] + b->NumberOfSat[s]; ++i) {
        T_GNSS_SATELLITE_ID(b->Sat[i].ID)
        T_NO_OF_BIASES(b->Sat[i].NumberOfPhaseBiases)
        T_YAW_ANGLE(b->Sat[i].YawAngle)
        T_YAW_RATE(b->Sat[i].YawRate)
        for (j = 0; j < b->Sat[i].NumberOfPhaseBiases; ++j) {
          T_GNSS_SIGNAL_IDENTIFIER(b->Sat[i].Biases[j].Type)
          T_INTEGER_INDICATOR(b->Sat[i].Biases[j].SignalIntegerIndicator ? 1 : 0)
          T_WIDE_LANE_INDICATOR(b->Sat[i].Biases[j].SignalsWideLaneIntegerIndicator)
          T_DISCONTINUITY_COUNTER(b->Sat[i].Biases[j].SignalDiscontinuityCounter)
          T_PHASE_BIAS(b->Sat[i].Biases[j].Bias)
        }
      }
      ENDBLOCK
    }
  }
  return ressize;
}

size_t SsrCorrIgs::MakeVTEC(const struct VTEC *v, int moremessagesfollow, char *buffer, size_t size) {
  unsigned int l, o, d;

  STARTDATA
    INITBLOCK

  T_RTCM_MESSAGE_NUMBER(RTCM_MESSAGE_NUMBER_IGS)
  T_IGS_SSR_VERSION(IGS_SSR_VERSION)
  T_IGS_MESSAGE_NUMBER(VTEC_BASE)
  T_SSR_EPOCH_TIME(v->EpochTime)
  T_SSR_UPDATE_INTERVAL(v->UpdateInterval)
  T_MULTIPLE_MESSAGE_INDICATOR(moremessagesfollow ? 1 : 0)
  T_SSR_IOD(v->SSRIOD)
  T_SSR_PROVIDER_ID(v->SSRProviderID)
  T_SSR_SOLUTION_ID(v->SSRSolutionID)
  T_VTEC_QUALITY_INDICATOR(v->Quality)
  T_NO_IONO_LAYERS(v->NumLayers)
  for (l = 0; l < v->NumLayers; ++l) {
    T_IONO_HEIGHT(v->Layers[l].Height)
    T_IONO_DEGREE(v->Layers[l].Degree)
    T_IONO_ORDER(v->Layers[l].Order)
    for (o = 0; o <= v->Layers[l].Order; ++o) {
      for (d = o; d <= v->Layers[l].Degree; ++d) {
        T_IONO_COEFF_C(v->Layers[l].Cosinus[d][o])
      }
    }
    for (o = 1; o <= v->Layers[l].Order; ++o) {
      for (d = o; d <= v->Layers[l].Degree; ++d) {
        T_IONO_COEFF_S(v->Layers[l].Sinus[d][o])
      }
    }
  }
  ENDBLOCK
  return ressize;
}

enum GCOB_RETURN SsrCorrIgs::GetSSR(struct ClockOrbit *co, struct CodeBias *b,struct VTEC *v,
    struct PhaseBias *pb, const char *buffer, size_t size, int *bytesused) {
  int mmi = 0, h, rs;
  unsigned int rtcmtype, igstype, pos, i, j, s, nums, id, version;
  size_t sizeofrtcmblock;
  const char *blockstart = buffer;
  DECODESTART

  if (size < 7)
    return GCOBR_SHORTBUFFER;

#ifdef BNC_DEBUG_SSR
  fprintf(stderr, "GetSSR-IGS START: size %d, numbits %d\n",(int)size, numbits);
#endif

  G_HEADER(h)
  G_RESERVEDH(rs)
  G_SIZE(sizeofrtcmblock);

  if ((unsigned char) h != 0xD3 || rs)
    return GCOBR_UNKNOWNDATA;
  if (size < sizeofrtcmblock + 3) /* 3 header bytes already removed */
    return GCOBR_MESSAGEEXCEEDSBUFFER;
  if (CRC24(sizeofrtcmblock + 3, (const unsigned char *) blockstart) !=
      (uint32_t) ((((unsigned char) buffer[sizeofrtcmblock]) << 16) |
          (((unsigned char) buffer[sizeofrtcmblock + 1]) << 8) |
          (((unsigned char) buffer[sizeofrtcmblock + 2]))))
    return GCOBR_CRCMISMATCH;
  size = sizeofrtcmblock; /* reduce size, so overflows are detected */

  G_RTCM_MESSAGE_NUMBER(rtcmtype)
  G_IGS_SSR_VERSION(version)
#ifdef BNC_DEBUG_SSR
  fprintf(stderr, "rtcmType %d igsVersion %d ", rtcmtype, version);
#endif
  G_IGS_MESSAGE_NUMBER(igstype)
#ifdef BNC_DEBUG_SSR
  fprintf(stderr, "igsType IM%d size %d\n", igstype,(int)sizeofrtcmblock);
#endif
  if (bytesused)
    *bytesused = sizeofrtcmblock + 6;

  if (igstype == VTEC_BASE) {
    unsigned int l, o, d;
    if (!v)
      return GCOBR_NOVTECPARAMETER;
    memset(v, 0, sizeof(*v));
    G_SSR_EPOCH_TIME(v->EpochTime)
    G_SSR_UPDATE_INTERVAL(v->UpdateInterval)
    G_MULTIPLE_MESSAGE_INDICATOR(mmi)
    G_SSR_IOD(v->SSRIOD)
    G_SSR_PROVIDER_ID(v->SSRProviderID)
    G_SSR_SOLUTION_ID(v->SSRSolutionID)
    G_VTEC_QUALITY_INDICATOR(v->Quality)
    G_NO_IONO_LAYERS(v->NumLayers)
#ifdef BNC_DEBUG_SSR
          fprintf(stderr, "epochTime %d ui %d mmi %d ssrIod %d providerId %d solId %d vtecQ %8.3f numLay %d \n",
        		  v->EpochTime, v->UpdateInterval, mmi,
				  v->SSRIOD, v->SSRProviderID, v->SSRSolutionID, v->Quality, v->NumLayers);
#endif
    for (l = 0; l < v->NumLayers; ++l) {
      G_IONO_HEIGHT(v->Layers[l].Height)
      G_IONO_DEGREE(v->Layers[l].Degree)
      G_IONO_ORDER(v->Layers[l].Order)
#ifdef BNC_DEBUG_SSR
          fprintf(stderr, "h  %8.3f deg %d ord %d \n",
        		  v->Layers[l].Height, v->Layers[l].Degree, v->Layers[l].Order);
#endif
      for (o = 0; o <= v->Layers[l].Order; ++o) {
        for (d = o; d <= v->Layers[l].Degree; ++d) {
          G_IONO_COEFF_C(v->Layers[l].Cosinus[d][o])
#ifdef BNC_DEBUG_SSR
          fprintf(stderr, "C[%02d][%02d]  %8.3f \n",
        		  d, o, v->Layers[l].Cosinus[d][o]);
#endif
        }
      }
      for (o = 1; o <= v->Layers[l].Order; ++o) {
        for (d = o; d <= v->Layers[l].Degree; ++d) {
          G_IONO_COEFF_S(v->Layers[l].Sinus[d][o])
#ifdef BNC_DEBUG_SSR
          fprintf(stderr, "S[%02d][%02d]  %8.3f \n",
        		  d, o, v->Layers[l].Sinus[d][o]);
#endif
        }
      }
    }
#ifdef BNC_DEBUG_SSR
    for(igstype = 0; igstype < (unsigned int)size && (unsigned char)buffer[igstype] != 0xD3; ++igstype)
    numbits += 8;
    fprintf(stderr, "numbits left %d\n",numbits);
#endif
    return mmi ? GCOBR_MESSAGEFOLLOWS : GCOBR_OK;
  }
  for (s = CLOCKORBIT_SATNUM; s-- > 0;) {
    if (igstype >= corbase[s]) {
      unsigned int corrOffset = igstype - corbase[s];
      if (corrOffset == COBOFS_ORBIT) {
          if (!co)
            return GCOBR_NOCLOCKORBITPARAMETER;
          co->messageType = igstype;
          G_SSR_EPOCH_TIME_CHECK(co->EpochTime[s], co->NumberOfSat[s])
          G_SSR_UPDATE_INTERVAL(co->UpdateInterval)
          G_MULTIPLE_MESSAGE_INDICATOR(mmi)
          G_SSR_IOD(co->SSRIOD)
          G_SSR_PROVIDER_ID(co->SSRProviderID)
          G_SSR_SOLUTION_ID(co->SSRSolutionID)
          G_SATELLITE_REFERENCE_DATUM(co->SatRefDatum)
          G_NO_OF_SATELLITES(nums)
          co->Supplied[COBOFS_ORBIT] |= 1;
#ifdef BNC_DEBUG_SSR
          fprintf(stderr, "epochTime %d ui %d mmi %d sats %d/%d rd %d ssrIod %d providerId %d solId %d\n",
        		  co->EpochTime[s], co->UpdateInterval,mmi,co->NumberOfSat[s],nums,
				  co->SatRefDatum, co->SSRIOD, co->SSRProviderID, co->SSRSolutionID);
#endif
          for (i = 0; i < nums; ++i) {
            G_GNSS_SATELLITE_ID(id)
            for (pos = satoffset[s];
                pos < satoffset[s] + co->NumberOfSat[s] && co->Sat[pos].ID != id;
                ++pos)
              ;
            if (pos >= satoffset[s + 1])
              return GCOBR_DATAMISMATCH;
            else if (pos == co->NumberOfSat[s] + satoffset[s])
              ++co->NumberOfSat[s];
            co->Sat[pos].ID = id;
            G_GNSS_IOD(co->Sat[pos].IOD)
            G_DELTA_RADIAL(co->Sat[pos].Orbit.DeltaRadial)
            G_DELTA_ALONG_TRACK(co->Sat[pos].Orbit.DeltaAlongTrack)
            G_DELTA_CROSS_TRACK(co->Sat[pos].Orbit.DeltaCrossTrack)
            G_DELTA_DOT_RADIAL(co->Sat[pos].Orbit.DotDeltaRadial)
            G_DELTA_DOT_ALONG_TRACK(co->Sat[pos].Orbit.DotDeltaAlongTrack)
            G_DELTA_DOT_CROSS_TRACK(co->Sat[pos].Orbit.DotDeltaCrossTrack)
#ifdef BNC_DEBUG_SSR
            fprintf(stderr, "id %2d iod %3d dr %8.4f da %8.4f dc %8.4f dr %8.3f da %8.3f dc %8.3f\n",
                co->Sat[pos].ID,co->Sat[pos].IOD,co->Sat[pos].Orbit.DeltaRadial,
                co->Sat[pos].Orbit.DeltaAlongTrack,co->Sat[pos].Orbit.DeltaCrossTrack,
                co->Sat[pos].Orbit.DotDeltaRadial,
                co->Sat[pos].Orbit.DotDeltaAlongTrack,
                co->Sat[pos].Orbit.DotDeltaCrossTrack);
#endif
          }
      }
      else if (corrOffset == COBOFS_CLOCK) {
          if (!co)
            return GCOBR_NOCLOCKORBITPARAMETER;
          co->messageType = igstype;
          G_SSR_EPOCH_TIME_CHECK(co->EpochTime[s], co->NumberOfSat[s])
          G_SSR_UPDATE_INTERVAL(co->UpdateInterval)
          G_MULTIPLE_MESSAGE_INDICATOR(mmi)
          G_SSR_IOD(co->SSRIOD)
          G_SSR_PROVIDER_ID(co->SSRProviderID)
          G_SSR_SOLUTION_ID(co->SSRSolutionID)
          G_NO_OF_SATELLITES(nums)
          co->Supplied[COBOFS_CLOCK] |= 1;
#ifdef BNC_DEBUG_SSR
          fprintf(stderr, "epochTime %d ui %d mmi %d sats %d/%d ssrIod %d providerId %d solId %d\n",
        		  co->EpochTime[s], co->UpdateInterval,mmi,co->NumberOfSat[s],nums,
				  co->SSRIOD, co->SSRProviderID, co->SSRSolutionID);
#endif
          for (i = 0; i < nums; ++i) {
            G_GNSS_SATELLITE_ID(id)
            for (pos = satoffset[s];
                pos < satoffset[s] + co->NumberOfSat[s] && co->Sat[pos].ID != id;
                ++pos)
              ;
            if (pos >= satoffset[s + 1])
              return GCOBR_DATAMISMATCH;
            else if (pos == co->NumberOfSat[s] + satoffset[s])
              ++co->NumberOfSat[s];
            co->Sat[pos].ID = id;
            G_DELTA_CLOCK_C0(co->Sat[pos].Clock.DeltaA0)
            G_DELTA_CLOCK_C1(co->Sat[pos].Clock.DeltaA1)
            G_DELTA_CLOCK_C2(co->Sat[pos].Clock.DeltaA2)
#ifdef BNC_DEBUG_SSR
            fprintf(stderr, "id %2d c0 %8.3f c1 %8.3f c2 %8.3f\n",
                co->Sat[pos].ID, co->Sat[pos].Clock.DeltaA0, co->Sat[pos].Clock.DeltaA1,
                co->Sat[pos].Clock.DeltaA2);
#endif
          }
      }
      else if (corrOffset == COBOFS_COMBINED) {
          if (!co)
            return GCOBR_NOCLOCKORBITPARAMETER;
          co->messageType = igstype;
          G_SSR_EPOCH_TIME_CHECK(co->EpochTime[s], co->NumberOfSat[s])
          G_SSR_UPDATE_INTERVAL(co->UpdateInterval)
          G_MULTIPLE_MESSAGE_INDICATOR(mmi)
          G_SSR_IOD(co->SSRIOD)
          G_SSR_PROVIDER_ID(co->SSRProviderID)
          G_SSR_SOLUTION_ID(co->SSRSolutionID)
          G_SATELLITE_REFERENCE_DATUM(co->SatRefDatum)
          G_NO_OF_SATELLITES(nums)
          co->Supplied[COBOFS_ORBIT] |= 1;
          co->Supplied[COBOFS_CLOCK] |= 1;
#ifdef BNC_DEBUG_SSR
          fprintf(stderr, "epochTime %d ui %d mmi %d sats %d/%d rd %d ssrIod %d providerId %d solId %d\n",
        		  co->EpochTime[s], co->UpdateInterval,mmi,co->NumberOfSat[s],nums,
				  co->SatRefDatum, co->SSRIOD, co->SSRProviderID, co->SSRSolutionID);
#endif
          for (i = 0; i < nums; ++i) {
            G_GNSS_SATELLITE_ID(id)
            for (pos = satoffset[s];
                pos < satoffset[s] + co->NumberOfSat[s] && co->Sat[pos].ID != id;
                ++pos)
              ;
            if (pos >= satoffset[s + 1])
              return GCOBR_DATAMISMATCH;
            else if (pos == co->NumberOfSat[s] + satoffset[s])
              ++co->NumberOfSat[s];
            co->Sat[pos].ID = id;
            G_GNSS_IOD(co->Sat[pos].IOD)
            G_DELTA_RADIAL(co->Sat[pos].Orbit.DeltaRadial)
            G_DELTA_ALONG_TRACK(co->Sat[pos].Orbit.DeltaAlongTrack)
            G_DELTA_CROSS_TRACK(co->Sat[pos].Orbit.DeltaCrossTrack)
            G_DELTA_DOT_RADIAL(co->Sat[pos].Orbit.DotDeltaRadial)
            G_DELTA_DOT_ALONG_TRACK(co->Sat[pos].Orbit.DotDeltaAlongTrack)
            G_DELTA_DOT_CROSS_TRACK(co->Sat[pos].Orbit.DotDeltaCrossTrack)
            G_DELTA_CLOCK_C0(co->Sat[pos].Clock.DeltaA0)
            G_DELTA_CLOCK_C1(co->Sat[pos].Clock.DeltaA1)
            G_DELTA_CLOCK_C2(co->Sat[pos].Clock.DeltaA2)
#ifdef BNC_DEBUG_SSR
            fprintf(stderr, "id %2d iod %3d dr %10.6f da %10.6f dc %10.6f dr %10.6f da %10.6f dc %10.6f  c0 %10.6f c1 %10.6f c2 %10.6f\n",
                co->Sat[pos].ID,co->Sat[pos].IOD,co->Sat[pos].Orbit.DeltaRadial,
                co->Sat[pos].Orbit.DeltaAlongTrack,co->Sat[pos].Orbit.DeltaCrossTrack,
                co->Sat[pos].Orbit.DotDeltaRadial, co->Sat[pos].Orbit.DotDeltaAlongTrack,
                co->Sat[pos].Orbit.DotDeltaCrossTrack,
			        	co->Sat[pos].Clock.DeltaA0, co->Sat[pos].Clock.DeltaA1, co->Sat[pos].Clock.DeltaA2);
#endif
          }
      }
      else if (corrOffset == COBOFS_URA) {
          if (!co)
            return GCOBR_NOCLOCKORBITPARAMETER;
          co->messageType = igstype;
          G_SSR_EPOCH_TIME_CHECK(co->EpochTime[s], co->NumberOfSat[s])
          G_SSR_UPDATE_INTERVAL(co->UpdateInterval)
          G_MULTIPLE_MESSAGE_INDICATOR(mmi)
          G_SSR_IOD(co->SSRIOD)
          G_SSR_PROVIDER_ID(co->SSRProviderID)
          G_SSR_SOLUTION_ID(co->SSRSolutionID)
          G_NO_OF_SATELLITES(nums)
          co->Supplied[COBOFS_URA] |= 1;
#ifdef BNC_DEBUG_SSR
          fprintf(stderr, "epochTime %d ui %d mmi %d sats %d/%d ssrIod %d providerId %d solId %d\n",
        		  co->EpochTime[s], co->UpdateInterval,mmi,co->NumberOfSat[s],nums,
				  co->SSRIOD, co->SSRProviderID, co->SSRSolutionID);
#endif
          for (i = 0; i < nums; ++i) {
            G_GNSS_SATELLITE_ID(id)
            for (pos = satoffset[s];
                pos < satoffset[s] + co->NumberOfSat[s] && co->Sat[pos].ID != id;
                ++pos)
              ;
            if (pos >= satoffset[s + 1])
              return GCOBR_DATAMISMATCH;
            else if (pos == co->NumberOfSat[s] + satoffset[s])
              ++co->NumberOfSat[s];
            co->Sat[pos].ID = id;
            G_SSR_URA(co->Sat[pos].UserRangeAccuracy)
#ifdef BNC_DEBUG_SSR
            fprintf(stderr, "id %2d ura %8.3f \n",
                co->Sat[pos].ID, co->Sat[pos].UserRangeAccuracy);
#endif
          }
      }
      else if (corrOffset == COBOFS_HR) {
          if (!co)
            return GCOBR_NOCLOCKORBITPARAMETER;
          co->messageType = igstype;
          G_SSR_EPOCH_TIME_CHECK(co->EpochTime[s], co->NumberOfSat[s])
          G_SSR_UPDATE_INTERVAL(co->UpdateInterval)
          G_MULTIPLE_MESSAGE_INDICATOR(mmi)
          G_SSR_IOD(co->SSRIOD)
          G_SSR_PROVIDER_ID(co->SSRProviderID)
          G_SSR_SOLUTION_ID(co->SSRSolutionID)
          G_NO_OF_SATELLITES(nums)
          co->Supplied[COBOFS_HR] |= 1;
#ifdef BNC_DEBUG_SSR
          fprintf(stderr, "epochTime %d ui %d mmi %d sats %d/%d ssrIod %d providerId %d solId %d\n",
        		  co->EpochTime[s], co->UpdateInterval,mmi,co->NumberOfSat[s],nums,
				  co->SSRIOD, co->SSRProviderID, co->SSRSolutionID);
#endif
          for (i = 0; i < nums; ++i) {
            G_GNSS_SATELLITE_ID(id)
            for (pos = satoffset[s];
                pos < satoffset[s] + co->NumberOfSat[s] && co->Sat[pos].ID != id;
                ++pos)
              ;
            if (pos >= satoffset[s + 1])
              return GCOBR_DATAMISMATCH;
            else if (pos == co->NumberOfSat[s] + satoffset[s])
              ++co->NumberOfSat[s];
            co->Sat[pos].ID = id;
            G_HR_CLOCK_CORRECTION(co->Sat[pos].hrclock)
#ifdef BNC_DEBUG_SSR
            fprintf(stderr, "id %2d hrClock %8.3f \n",
                    co->Sat[pos].ID, co->Sat[pos].hrclock);
#endif
          }
      }
      else if (corrOffset == COBOFS_CBIAS) {
          if (!b)
            return GCOBR_NOCODEBIASPARAMETER;
          b->messageType = igstype;
          G_SSR_EPOCH_TIME_CHECK(b->EpochTime[s], b->NumberOfSat[s])
          G_SSR_UPDATE_INTERVAL(b->UpdateInterval)
          G_MULTIPLE_MESSAGE_INDICATOR(mmi)
          G_SSR_IOD(b->SSRIOD)
          G_SSR_PROVIDER_ID(b->SSRProviderID)
          G_SSR_SOLUTION_ID(b->SSRSolutionID)
          G_NO_OF_SATELLITES(nums)
#ifdef BNC_DEBUG_SSR
          fprintf(stderr, "epochTime %d ui %d mmi %d sats %d/%d ssrIod %d providerId %d solId %d\n",
        		  b->EpochTime[s], b->UpdateInterval,mmi,b->NumberOfSat[s],nums,
				  b->SSRIOD, b->SSRProviderID, b->SSRSolutionID);
#endif
          for (i = 0; i < nums; ++i) {
            G_GNSS_SATELLITE_ID(id)
            for (pos = satoffset[s];
                pos < satoffset[s] + b->NumberOfSat[s] && b->Sat[pos].ID != id;
                ++pos)
              ;
            if (pos >= satoffset[s + 1])
              return GCOBR_DATAMISMATCH;
            else if (pos == b->NumberOfSat[s] + satoffset[s])
              ++b->NumberOfSat[s];
            b->Sat[pos].ID = id;
            G_NO_OF_BIASES(b->Sat[pos].NumberOfCodeBiases)
#ifdef BNC_DEBUG_SSR
            fprintf(stderr, "id %2d #%d ",
                    b->Sat[pos].ID, b->Sat[pos].NumberOfCodeBiases);
#endif
            for (j = 0; j < b->Sat[pos].NumberOfCodeBiases; ++j) {
              G_GNSS_SIGNAL_IDENTIFIER(b->Sat[pos].Biases[j].Type)
              G_CODE_BIAS(b->Sat[pos].Biases[j].Bias)
#ifdef BNC_DEBUG_SSR
            fprintf(stderr, "t%02d b %8.2f ",
                    b->Sat[pos].Biases[j].Type, b->Sat[pos].Biases[j].Bias);
#endif
            }
#ifdef BNC_DEBUG_SSR
            fprintf(stderr, "\n");
#endif
          }
      }
      else if (corrOffset == COBOFS_PBIAS) {
            if (!pb)
              return GCOBR_NOPHASEBIASPARAMETER;
            pb->messageType = igstype;
            G_SSR_EPOCH_TIME_CHECK(pb->EpochTime[s], pb->NumberOfSat[s])
            G_SSR_UPDATE_INTERVAL(pb->UpdateInterval)
            G_MULTIPLE_MESSAGE_INDICATOR(mmi)
            G_SSR_IOD(pb->SSRIOD)
            G_SSR_PROVIDER_ID(pb->SSRProviderID)
            G_SSR_SOLUTION_ID(pb->SSRSolutionID)
            G_DISPERSIVE_BIAS_INDICATOR(pb->DispersiveBiasConsistencyIndicator)
            G_MW_CONSISTENCY_INDICATOR(pb->MWConsistencyIndicator)
            G_NO_OF_SATELLITES(nums)
#ifdef BNC_DEBUG_SSR
          fprintf(stderr, "epochTime %d ui %d mmi %d sats %d/%d ssrIod %d providerId %d solId %d dispInd %d mwInd %d\n",
        		  pb->EpochTime[s], pb->UpdateInterval,mmi,pb->NumberOfSat[s],nums,
				  pb->SSRIOD, pb->SSRProviderID, pb->SSRSolutionID,
				  pb->DispersiveBiasConsistencyIndicator, pb->MWConsistencyIndicator);
#endif
            for (i = 0; i < nums; ++i) {
              G_GNSS_SATELLITE_ID(id)
              for (pos = satoffset[s];
                  pos < satoffset[s] + pb->NumberOfSat[s] && pb->Sat[pos].ID != id;
                  ++pos)
                ;
              if (pos >= satoffset[s + 1])
                return GCOBR_DATAMISMATCH;
              else if (pos == pb->NumberOfSat[s] + satoffset[s])
                ++pb->NumberOfSat[s];
              pb->Sat[pos].ID = id;
              G_NO_OF_BIASES(pb->Sat[pos].NumberOfPhaseBiases)
              G_YAW_ANGLE(pb->Sat[pos].YawAngle)
              G_YAW_RATE(pb->Sat[pos].YawRate)
#ifdef BNC_DEBUG_SSR
            fprintf(stderr, "id %2d #%d y %10.6f yr %10.6f ",
                    pb->Sat[pos].ID, pb->Sat[pos].NumberOfPhaseBiases,
					pb->Sat[pos].YawAngle/MPI, pb->Sat[pos].YawRate/MPI);
#endif
              for (j = 0; j < pb->Sat[pos].NumberOfPhaseBiases; ++j) {
                G_GNSS_SIGNAL_IDENTIFIER(pb->Sat[pos].Biases[j].Type)
                G_INTEGER_INDICATOR(pb->Sat[pos].Biases[j].SignalIntegerIndicator)
                G_WIDE_LANE_INDICATOR(pb->Sat[pos].Biases[j].SignalsWideLaneIntegerIndicator)
                G_DISCONTINUITY_COUNTER(pb->Sat[pos].Biases[j].SignalDiscontinuityCounter)
                G_PHASE_BIAS(pb->Sat[pos].Biases[j].Bias)
#ifdef BNC_DEBUG_SSR
            fprintf(stderr, "t%02d int %d wl %d disc %02d b %8.4f ",
                    pb->Sat[pos].Biases[j].Type,
					pb->Sat[pos].Biases[j].SignalIntegerIndicator,
					pb->Sat[pos].Biases[j].SignalsWideLaneIntegerIndicator,
					pb->Sat[pos].Biases[j].SignalDiscontinuityCounter,
					pb->Sat[pos].Biases[j].Bias);
#endif
              }
#ifdef BNC_DEBUG_SSR
            fprintf(stderr, "\n");
#endif
            }
      }
      else {
        continue;
      }
#ifdef BNC_DEBUG_SSR
      for(igstype = 0; igstype < (unsigned int)size && (unsigned char)buffer[igstype] != 0xD3; ++igstype)
      numbits += 8;
      fprintf(stderr, "numbits left %d\n",numbits);
#endif
      return mmi ? GCOBR_MESSAGEFOLLOWS : GCOBR_OK;
    }
  }
  return GCOBR_UNKNOWNTYPE;
}

//
////////////////////////////////////////////////////////////////////////////
std::string SsrCorrIgs::codeTypeToRnxType(char system, CodeType type) {
  if      (system == 'G') {
    if (type == CODETYPE_GPS_L1_CA)         return "1C";
    if (type == CODETYPE_GPS_L1_P)          return "1P";
    if (type == CODETYPE_GPS_L1_Z)          return "1W";

    if (type == CODETYPE_GPS_SEMI_CODELESS) return "2D";
    if (type == CODETYPE_GPS_L2_CA)         return "2C";
    if (type == CODETYPE_GPS_L2_P)          return "2P";
    if (type == CODETYPE_GPS_L2_Z)          return "2W";
    if (type == CODETYPE_GPS_L2C_M)         return "2S";
    if (type == CODETYPE_GPS_L2C_L)         return "2L";

    if (type == CODETYPE_GPS_L5_I)          return "5I";
    if (type == CODETYPE_GPS_L5_Q)          return "5Q";

    if (type == CODETYPE_GPS_L1C_D)         return "1S";
    if (type == CODETYPE_GPS_L1C_P)         return "1L";
  }
  else if (system == 'R') {
    if (type == CODETYPE_GLONASS_L1_CA)     return "1C";
    if (type == CODETYPE_GLONASS_L1_P)      return "1P";

    if (type == CODETYPE_GLONASS_L2_CA)     return "2C";
    if (type == CODETYPE_GLONASS_L2_P)      return "2P";

    if (type == CODETYPE_GLONASS_L1a_OCd)   return "4A";
    if (type == CODETYPE_GLONASS_L1a_OCp)   return "4B";

    if (type == CODETYPE_GLONASS_L2a_CSI)   return "6A";
    if (type == CODETYPE_GLONASS_L2a_OCp)   return "6B";

    if (type == CODETYPE_GLONASS_L3_I)      return "3I";
    if (type == CODETYPE_GLONASS_L3_Q)      return "3Q";
  }
  else if (system == 'E') {
    if (type == CODETYPE_GALILEO_E1_A)       return "1A";
    if (type == CODETYPE_GALILEO_E1_B)       return "1B";
    if (type == CODETYPE_GALILEO_E1_C)       return "1C";

    if (type == CODETYPE_GALILEO_E5A_I)      return "5I";
    if (type == CODETYPE_GALILEO_E5A_Q)      return "5Q";

    if (type == CODETYPE_GALILEO_E5B_I)      return "7I";
    if (type == CODETYPE_GALILEO_E5B_Q)      return "7Q";

    if (type == CODETYPE_GALILEO_E6_A)       return "6A";
    if (type == CODETYPE_GALILEO_E6_B)       return "6B";
    if (type == CODETYPE_GALILEO_E6_C)       return "6C";
  }
   else if (system == 'J') {
    if (type == CODETYPE_QZSS_L1_CA)         return "1C";
    if (type == CODETYPE_QZSS_L1C_D)         return "1S";
    if (type == CODETYPE_QZSS_L1C_P)         return "1L";

    if (type == CODETYPE_QZSS_L2C_M)         return "2S";
    if (type == CODETYPE_QZSS_L2C_L)         return "2L";

    if (type == CODETYPE_QZSS_L5_I)          return "5I";
    if (type == CODETYPE_QZSS_L5_Q)          return "5Q";

    if (type == CODETYPE_QZSS_L6_D)          return "6S";
    if (type == CODETYPE_QZSS_L6_P)          return "6L";
    if (type == CODETYPE_QZSS_L6_E)          return "6E";
  }
  else if (system == 'C') {
    if (type == CODETYPE_BDS_B1_I)         return "2I";
    if (type == CODETYPE_BDS_B1_Q)         return "2Q";

    if (type == CODETYPE_BDS_B3_I)         return "6I";
    if (type == CODETYPE_BDS_B3_Q)         return "6Q";
    if (type == CODETYPE_BDS_B3_A)         return "6A";

    if (type == CODETYPE_BDS_B2_I)         return "7I";
    if (type == CODETYPE_BDS_B2_Q)         return "7Q";

    if (type == CODETYPE_BDS_B1a_D)        return "1D";
    if (type == CODETYPE_BDS_B1a_P)        return "1P";

    if (type == CODETYPE_BDS_B2a_D)        return "5D";
    if (type == CODETYPE_BDS_B2a_P)        return "5P";

    if (type == CODETYPE_BDS_B1_A)         return "1A";
  }
  else if (system == 'S') {
    if (type == CODETYPE_SBAS_L1_CA)       return "1C";

    if (type == CODETYPE_SBAS_L5_I)        return "5I";
    if (type == CODETYPE_SBAS_L5_Q)        return "5Q";
  }
  return "";
}

SsrCorr::CodeType SsrCorrIgs::rnxTypeToCodeType(char system, std::string type) {
  if      (system == 'G') {
    if (type.compare("1C") == 0) {return CODETYPE_GPS_L1_CA;}
    if (type.compare("1P") == 0) {return CODETYPE_GPS_L1_P;}
    if (type.compare("1W") == 0) {return CODETYPE_GPS_L1_Z;}

    if (type.compare("2D") == 0) {return CODETYPE_GPS_SEMI_CODELESS;}
    if (type.compare("2C") == 0) {return CODETYPE_GPS_L2_CA;}
    if (type.compare("2P") == 0) {return CODETYPE_GPS_L2_P;}
    if (type.compare("2W") == 0) {return CODETYPE_GPS_L2_Z;}
    if (type.compare("2S") == 0) {return CODETYPE_GPS_L2C_M;}
    if (type.compare("2L") == 0) {return CODETYPE_GPS_L2C_L;}

    if (type.compare("5I") == 0) {return CODETYPE_GPS_L5_I;}
    if (type.compare("5Q") == 0) {return CODETYPE_GPS_L5_Q;}

    if (type.compare("1S") == 0) {return CODETYPE_GPS_L1C_D;}
    if (type.compare("1L") == 0) {return CODETYPE_GPS_L1C_P;}
  }
  else if (system == 'R') {
    if (type.compare("1C") == 0) {return CODETYPE_GLONASS_L1_CA;}
    if (type.compare("1P") == 0) {return CODETYPE_GLONASS_L1_P;}

    if (type.compare("2C") == 0) {return CODETYPE_GLONASS_L2_CA;}
    if (type.compare("2P") == 0) {return CODETYPE_GLONASS_L2_P;}

    if (type.compare("4A") == 0) {return CODETYPE_GLONASS_L1a_OCd;}
    if (type.compare("4B") == 0) {return CODETYPE_GLONASS_L1a_OCp;}

    if (type.compare("6A") == 0) {return CODETYPE_GLONASS_L2a_CSI;}
    if (type.compare("6B") == 0) {return CODETYPE_GLONASS_L2a_OCp;}

    if (type.compare("3I") == 0) {return CODETYPE_GLONASS_L3_I;}
    if (type.compare("3Q") == 0) {return CODETYPE_GLONASS_L3_Q;}
  }
  else if (system == 'E') {
    if (type.compare("1A") == 0) {return CODETYPE_GALILEO_E1_A;}
    if (type.compare("1B") == 0) {return CODETYPE_GALILEO_E1_B;}
    if (type.compare("1C") == 0) {return CODETYPE_GALILEO_E1_C;}

    if (type.compare("5I") == 0) {return CODETYPE_GALILEO_E5A_I;}
    if (type.compare("5Q") == 0) {return CODETYPE_GALILEO_E5A_Q;}

    if (type.compare("7I") == 0) {return CODETYPE_GALILEO_E5B_I;}
    if (type.compare("7Q") == 0) {return CODETYPE_GALILEO_E5B_Q;}

    if (type.compare("6A") == 0) {return CODETYPE_GALILEO_E6_A;}
    if (type.compare("6B") == 0) {return CODETYPE_GALILEO_E6_B;}
    if (type.compare("6C") == 0) {return CODETYPE_GALILEO_E6_C;}
  }
   else if (system == 'J') {
     if (type.compare("1C") == 0) {return CODETYPE_QZSS_L1_CA;}
     if (type.compare("1S") == 0) {return CODETYPE_QZSS_L1C_D;}
     if (type.compare("1L") == 0) {return CODETYPE_QZSS_L1C_P;}

     if (type.compare("2S") == 0) {return CODETYPE_QZSS_L2C_M;}
     if (type.compare("2L") == 0) {return CODETYPE_QZSS_L2C_L;}

     if (type.compare("5I") == 0) {return CODETYPE_QZSS_L5_I;}
     if (type.compare("5Q") == 0) {return CODETYPE_QZSS_L5_Q;}

     if (type.compare("6S") == 0) {return CODETYPE_QZSS_L6_D;}
     if (type.compare("6L") == 0) {return CODETYPE_QZSS_L6_P;}
     if (type.compare("6E") == 0) {return CODETYPE_QZSS_L6_E;}
  }
  else if (system == 'C') {
    if (type.compare("2I") == 0) {return CODETYPE_BDS_B1_I;}
    if (type.compare("2Q") == 0) {return CODETYPE_BDS_B1_Q;}

    if (type.compare("6I") == 0) {return CODETYPE_BDS_B3_I;}
    if (type.compare("6Q") == 0) {return CODETYPE_BDS_B3_Q;}
    if (type.compare("6A") == 0) {return CODETYPE_BDS_B3_A;}

    if (type.compare("7I") == 0) {return CODETYPE_BDS_B2_I;}
    if (type.compare("7Q") == 0) {return CODETYPE_BDS_B2_Q;}

    if (type.compare("1D") == 0) {return CODETYPE_BDS_B1a_D;}
    if (type.compare("1P") == 0) {return CODETYPE_BDS_B1a_P;}

    if (type.compare("5D") == 0) {return CODETYPE_BDS_B2a_D;}
    if (type.compare("5P") == 0) {return CODETYPE_BDS_B2a_P;}

    if (type.compare("1A") == 0) {return CODETYPE_BDS_B1_A;}
  }
  else if (system == 'S') {
    if (type.compare("1C") == 0) {return CODETYPE_SBAS_L1_CA;}

    if (type.compare("5I") == 0) {return CODETYPE_SBAS_L5_I;}
    if (type.compare("5Q") == 0) {return CODETYPE_SBAS_L5_Q;}
  }
  return RESERVED;
}

