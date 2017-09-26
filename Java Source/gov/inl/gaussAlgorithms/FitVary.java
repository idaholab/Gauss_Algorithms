/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: FitVary.java
 *
 *  Description: holds information about which variable is allowed to vary
 */
package gov.inl.gaussAlgorithms;

import gov.inl.gaussAlgorithms.FitParameters.PeakWidthMode;
import gov.inl.gaussAlgorithms.FitInfo.PeakInfo;

import java.util.Iterator;
import java.util.Vector;

/**
 * indicates which values in the corresponding FitInfo can vary
 *
 */
public class FitVary {
	
	private final boolean            m_interceptVaries;
	private final boolean            m_slopeVaries;
	private final boolean            m_avgWidthVaries;
	private final Vector<PeakVary>   m_peakVaryList;
	
	FitVary(PeakWidthMode peakWidthMode, final FitInfo fitInfo) {
		
		// see cycle.c:CY_init_fitvary

		m_interceptVaries = true;
		m_slopeVaries = true;
					
		m_avgWidthVaries = getAvgWidthVaries(peakWidthMode, fitInfo);
		
		m_peakVaryList = new Vector<PeakVary>();
		for (Iterator<PeakInfo> it = fitInfo.getPeakIterator();
			 it.hasNext(); ) {
			PeakVary peakVary = new PeakVary(peakWidthMode,
					fitInfo.getPeakCount(), it.next());
			m_peakVaryList.add(peakVary);
		}
	}
	
	// public methods
	
	boolean avgWidthVaries() {
		return m_avgWidthVaries;
	}
	
	boolean bckInterceptVaries() {
		return m_interceptVaries;
	}
	
	boolean bckSlopeVaries() {
		return m_slopeVaries;
	}
	
	Iterator<PeakVary> getPeakIterator() {
		return m_peakVaryList.iterator();
	}
	
	int getVaryCount() {
		
		int count = 0;
		if (m_interceptVaries) count++;
		if (m_slopeVaries) count++;
		if (m_avgWidthVaries) count++;
		for (Iterator<PeakVary> it = m_peakVaryList.iterator();
			 it.hasNext(); ) {
			count += it.next().varyCount();
		}
		
		return count;
	}
	
	// private methods
	
	private static boolean getAvgWidthVaries(PeakWidthMode peakWidthMode,
			final FitInfo fitInfo) {
		
		boolean avgWidthVaries = false;
		for (Iterator<PeakInfo> it = fitInfo.getPeakIterator();
			 it.hasNext(); ) {
			PeakInfo peakInfo = it.next();
			
			if ((PeakWidthMode.VARIES.equals(peakWidthMode)) &&
				(!peakInfo.isFixedCentroid())) {
				avgWidthVaries = true;
				break;
			}
		}
		
		return avgWidthVaries;
	}

	// inner class
	
	/**
	 * indicates which values in the corresponding FitInfo.PeakInfo can vary
	 *
	 */
	public static class PeakVary {
		
		private final boolean   m_heightVaries;
		private final boolean   m_centroidVaries;
		private final boolean   m_addWidth511Varies;
		
		private PeakVary(PeakWidthMode peakWidthMode,
				int peakCount, PeakInfo peakInfo) {
			
			// see cycle.c:CY_init_fitvary and checkfit.c:CK_add_peak
			// and checkfit.c:CK_dlt_peak
			
			m_heightVaries = true;
			
			if (!peakInfo.isFixedCentroid()) {
				m_centroidVaries = true;
			} else {
				m_centroidVaries = false;
			}
			
			if ((0 != peakInfo.getAddWidth511Channels()) &&
				((1 < peakCount) ||
				 (PeakWidthMode.FIXED.equals(peakWidthMode)))) {
				m_addWidth511Varies = true;
			} else {
				m_addWidth511Varies = false;
			}
		}
		
		boolean addWidth511Varies() {
			return m_addWidth511Varies;
		}
		
		boolean centroidChannelsVaries() {
			return m_centroidVaries;
		}
		
		boolean heightCountsVaries() {
			return m_heightVaries;
		}

		int varyCount() {
			
			int count = 0;
			if (m_heightVaries) count++;
			if (m_centroidVaries) count++;
			if (m_addWidth511Varies) count++;
			
			return count;
		}
	} // PeakVary
}
