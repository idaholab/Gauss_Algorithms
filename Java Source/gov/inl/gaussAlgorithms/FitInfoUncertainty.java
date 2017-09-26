/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: FitInfoUncertainty.java
 *
 *  Description: holds uncertainties resulting from the fit
 */
package gov.inl.gaussAlgorithms;

import gov.inl.gaussAlgorithms.FitInfo.PeakInfo;
import gov.inl.gaussAlgorithms.FitVary.PeakVary;

import java.util.Iterator;
import java.util.Vector;

import org.apache.commons.math3.linear.RealMatrix;

/**
 * holds uncertainties of a FitInfo
 *
 */
public class FitInfoUncertainty {

	// member data
	
	private final double                         m_bckIntUncert;
	private final double                         m_bckSlopeUncert;
	private final double                         m_backgroundCovariance;
	private final double                         m_avgWidUncert;
	private final Vector<PeakInfoUncertainty>    m_peakInfoList;

	// constructor
	
	FitInfoUncertainty(final FitInfo fitInfo, FitVary fitVary,
			final RealMatrix covariances, EnergyEquation ex) {
		
		// covariances is fitVaryCount x fitVaryCount large
		
		/*
		 * see cycle.c: CY_nllsqs & CY_store_covar.
		 */
		
		int i = 0;
				
		int bkIntIndex = -1;
		if (fitVary.bckInterceptVaries()) {
			m_bckIntUncert = Math.sqrt(covariances.getEntry(i, i));
			bkIntIndex = i;
			i++;
		} else {
			m_bckIntUncert = 0;
		}
		
		int bkSlopeIndex = -1;
		if (fitVary.bckSlopeVaries()) {
			m_bckSlopeUncert = Math.sqrt(covariances.getEntry(i, i));
			bkSlopeIndex = i;
			i++;
		} else {
			m_bckSlopeUncert = 0;
		}
		
		// fitVary.bckInterceptVaries & fitVary.bckSlopeVaries
		if ((-1 != bkIntIndex) && (-1 != bkSlopeIndex)) {
			// Blackwood 1/3/2003 instructed that covariance of background
			// terms is located in either 1,0 or 0,1 because
			// row & col 0 are for background intercept
			// and row & col 1 are for background slope
			m_backgroundCovariance =
					covariances.getEntry(bkIntIndex, bkSlopeIndex);
		} else {
			m_backgroundCovariance = 0;
		}
		
		int avgWidIndex = -1;
		if (fitVary.avgWidthVaries()) {
			m_avgWidUncert = Math.sqrt(covariances.getEntry(i, i));
			avgWidIndex = i;
			i++;
		} else {
			m_avgWidUncert = 0;
		}
						
		m_peakInfoList = new Vector<PeakInfoUncertainty>();
		Iterator<PeakInfo> pit = fitInfo.getPeakIterator();
		Iterator<PeakVary> pvt = fitVary.getPeakIterator();
		for ( ; pit.hasNext() && pvt.hasNext(); ) {
			PeakInfo peakInfo = pit.next();
			PeakVary peakVary = pvt.next();
			
			double heightCountsUncert = 0;
			double centroidChannelsUncert = 0;
			double addWidth511ChannelsUncert = 0;
			double coVaryAvgWidPkHeight = 0;
			double coVaryAvgWidPkAddWid511 = 0;
			double coVaryPkHeightPkAddWid511 = 0;
			
			int pkHeightIndex = -1;
			if (peakVary.heightCountsVaries()) {
				heightCountsUncert = Math.sqrt(covariances.getEntry(i, i));
				pkHeightIndex = i;
				
				// fitVary.avgWidthVaries
				if (-1 != avgWidIndex) {
					coVaryAvgWidPkHeight =
							covariances.getEntry(avgWidIndex, i);
				}
				i++;
			}
			
			if (peakVary.centroidChannelsVaries()) {
				centroidChannelsUncert = Math.sqrt(covariances.getEntry(i, i));
				i++;
			}
			
			if (peakVary.addWidth511Varies()) {
				addWidth511ChannelsUncert =
						Math.sqrt(covariances.getEntry(i, i));
				
				// fitVary.avgWidthVaries
				if (-1 != avgWidIndex) {
					coVaryAvgWidPkAddWid511 =
							covariances.getEntry(avgWidIndex, i);
				}
				// peakVary.heightCountsVaries
				if (-1 != pkHeightIndex) {
					coVaryPkHeightPkAddWid511 =
							covariances.getEntry(pkHeightIndex, i);
				}
				i++;
			}
			
			PeakInfoUncertainty peakInfoUncertainty =
					new PeakInfoUncertainty(peakInfo, m_avgWidUncert,
							heightCountsUncert, centroidChannelsUncert,
							addWidth511ChannelsUncert,
							coVaryAvgWidPkHeight, coVaryAvgWidPkAddWid511,
							coVaryPkHeightPkAddWid511, 
							ex);
			m_peakInfoList.add(peakInfoUncertainty);
		}
	}
	
	// package methods
	
	double getAvgWidUncert() {
		return m_avgWidUncert;
	}
	double getBckCovariance() {
		return m_backgroundCovariance;
	}
	double getBckInterceptUncert() {
		return m_bckIntUncert;
	}
	double getBckSlopeUncert() {
		return m_bckSlopeUncert;
	}

	Iterator<PeakInfoUncertainty> getPeakIterator() {
		return m_peakInfoList.iterator();
	}
	
	// inner class
	
	/**
	 * holds uncertainties of a FitInfo.PeakInfo
	 *
	 */
	public static class PeakInfoUncertainty {
		// member data
		private final double       m_heightCountsUncert;
		private final double       m_centroidChannelsUncert;
		private final double       m_addWidth511ChannelsUncert;
		private final double       m_centroidEnergyUncert;
		private final double       m_fwhmUncert;
		private final double       m_areaUncert;
		
		// constructor

		private PeakInfoUncertainty(final PeakInfo peakInfo,
				double avgWidUncert, double heightCountsUncert,
				double centroidChannelsUncert,
				double addWidth511ChannelsUncert, double coVaryAvgWidPkHeight,
				double coVaryAvgWidPkAddWid511,
				double coVaryPkHeightPkAddWid511,
				EnergyEquation ex) {
			
			m_heightCountsUncert = heightCountsUncert;
			m_centroidChannelsUncert = centroidChannelsUncert;
			m_addWidth511ChannelsUncert = addWidth511ChannelsUncert;
			
			m_centroidEnergyUncert = computeCentroidEnergyUncertainty(
					peakInfo, ex);
			
			// see output.c summarize()
			//
			// covarr[k][j] stored in fit - extracted from covariances
			// k = peak count
			// j = 3
			// for each peak, three covariances are used:
			// covarr[i1][i2]: average width & peak height
			// covarr[i1][i3]: average width & peak additional width 511
			// covarr[i2][i3]: peak height & peak additional width 511
			
			double t1 = (addWidth511ChannelsUncert *
					     addWidth511ChannelsUncert) +
					(avgWidUncert * avgWidUncert) +
					(2.0 * coVaryAvgWidPkAddWid511);

			double t2 = heightCountsUncert * heightCountsUncert;

			double t3 = coVaryPkHeightPkAddWid511 + coVaryAvgWidPkHeight;

			double height = peakInfo.getHeightCounts();
			double width = peakInfo.getFwhm();
			double sta = FitInfo.AREA_FACTOR_SQUARED *
					((height * height * t1) + (width * width * t2) +
					 (2.0 * width * height * t3));

			if (sta >= 0.0) {
				m_areaUncert = Math.sqrt(sta);
			} else {
				m_areaUncert = 0;
			}
			if (t1 >= 0.0) {
				m_fwhmUncert = Math.sqrt(t1);
		    } else {
		    	m_fwhmUncert = 0;
		    }			
		}
				
		// package methods
		
		double getAddWidth511ChannelsUncert() {
			return m_addWidth511ChannelsUncert;
		}
		double getAreaUncert() {
			return m_areaUncert;
		}
		double getCentroidChannelsUncert() {
			return m_centroidChannelsUncert;
		}
		double getCentroidEnergyUncert(EnergyEquation ex) {
			return m_centroidEnergyUncert;
		}
		double getFwhmUncert() {
			return m_fwhmUncert;
		}
		double getHeightCountsUncert() {
			return m_heightCountsUncert;
		}
		
		// private methods
		
		private double computeCentroidEnergyUncertainty(
				final PeakInfo peakInfo, EnergyEquation ex) {
			
			double sige = (ex.getLinearCoefficient() +
			 (2 * peakInfo.getCentroidChannels() * ex.getQuadCoefficient())) *
			 m_centroidChannelsUncert;
			
			return sige;
		}
	} // PeakInfoUncertainty
}
