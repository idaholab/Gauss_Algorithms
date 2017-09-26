/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: FitInfo.java
 *
 *  Description: holds input information about the fit task
 */
package gov.inl.gaussAlgorithms;

import gov.inl.gaussAlgorithms.FitVary.PeakVary;

import java.util.Iterator;
import java.util.TreeSet;
import java.util.Vector;

import org.apache.commons.math3.linear.RealVector;

/**
 * holds information used by the region fit algorithm
 *
 */
public class FitInfo {
	
	private final static double PK511KEV_THRESHOLD = .6;
	public final static double AREA_FACTOR_SQUARED =
			Math.PI / (4 * Math.log(2));
	
	// member data
	
	private double                   m_backgroundIntercept;
	private double                   m_backgroundSlope;
	private double                   m_averagePeakWidthChannels;
	private boolean                  m_contains511KeV;
	private final double             m_initialPeakWidthChannels;
	private final Vector<PeakInfo>   m_peakInfoList;
	
	// constructors
	
	// for use by clone()
	private FitInfo(double backgroundIntercept, double backgroundSlope,
			double averagePeakWidthChannels, boolean contains511KeV,
			double initialPeakWidthChannels,
			final Vector<PeakInfo> initialPeaksList) {
		
		m_backgroundIntercept = backgroundIntercept;
		m_backgroundSlope = backgroundSlope;
		m_averagePeakWidthChannels = averagePeakWidthChannels;
		m_contains511KeV = contains511KeV;
		m_initialPeakWidthChannels = initialPeakWidthChannels;
		
		m_peakInfoList = new Vector<PeakInfo>();
		for (Iterator<PeakInfo> it = initialPeaksList.iterator();
			 it.hasNext(); ) {
			m_peakInfoList.add(it.next().clone());
		}

	}
	
	FitInfo(Spectrum spectrum, ChannelRange region, EnergyEquation ex,
			WidthEquation wx, final TreeSet<Peak> peaks) {
				
		m_contains511KeV = false;
		
		m_backgroundIntercept =
				((double) spectrum.getCountAt(region.getLastChannel() - 1) +
				 (double) spectrum.getCountAt(region.getLastChannel())) / 2.0;
		m_backgroundSlope = 0;
		
		// 8/18/2000 - Larry Blackwood noted that as originally
		//             calculated, xmid was rounded to the closest
		//             channel.  He said that floating point was the
		//             "right" thing to calculate here.
		//             So I changed it.
		double xmid = (region.getFirstChannel() +
						region.getLastChannel() + 1) / 2;
		
		m_averagePeakWidthChannels = 1;
		try {
			m_averagePeakWidthChannels = wx.getPeakwidth(xmid);
		} catch (Exception e) {
			// leave default value
		}
		m_initialPeakWidthChannels = m_averagePeakWidthChannels;
		
		Vector<Peak> useablePeaks = new Vector<Peak>();
		for (Iterator<Peak> it = peaks.iterator(); it.hasNext(); ) {
			Peak peak = it.next();
			if (peak.isChannelValid()) {
				int roundedChannel = (int) Math.round(peak.getChannel());
				if ((region.getFirstChannel() <= roundedChannel) &&
					(region.getLastChannel() >= roundedChannel)) {
					useablePeaks.add(peak);
				}
			}
		}
		
		m_peakInfoList = new Vector<PeakInfo>();
		for (Iterator<Peak> it = useablePeaks.iterator(); it.hasNext(); ) {
			Peak peak = it.next();
			
			int roundedChannel = (int) Math.round(peak.getChannel());
			double heightCounts = spectrum.getCountAt(roundedChannel) -
					m_backgroundIntercept;
			double addWidth511Channels = 0;
			if (!m_contains511KeV) {
				if (isNear511KeV(ex.getEnergy(peak.getChannel()))) {
					m_contains511KeV = true;
					addWidth511Channels = m_averagePeakWidthChannels;
				}
			}
			PeakInfo peakInfo = new PeakInfo(peak.getChannel(),
					heightCounts, addWidth511Channels,
					peak.isCentroidFixed(), m_averagePeakWidthChannels);
			m_peakInfoList.add(peakInfo);
		}
		
		// If no user peaks to add, then add one at the highest count
		// in the region. But if this is too close to region ends, then
		// add a peak at the region midpoint.
		
		int lastChannel = region.getLastChannel();
		int chanOfMaxCounts = 0;
		int maxCounts = 0;
		if (0 >= m_peakInfoList.size()) {
			for (int i = region.getFirstChannel(); i <= lastChannel; i++) {
				int chanCounts = spectrum.getCountAt(i);
				if (chanCounts > maxCounts) {
					chanOfMaxCounts = i;
					maxCounts = chanCounts;
				}
			}
			
			if ((chanOfMaxCounts < region.getFirstChannel() + 2) ||
				(chanOfMaxCounts > region.getLastChannel() - 2)) {
				chanOfMaxCounts = (region.getFirstChannel() +
						region.getLastChannel()) / 2;
				maxCounts = spectrum.getCountAt(chanOfMaxCounts);
			}
			
			double addWidth511Channels = 0;
			if (!m_contains511KeV) {
				if (isNear511KeV(ex.getEnergy(chanOfMaxCounts))) {
					m_contains511KeV = true;
					addWidth511Channels = m_averagePeakWidthChannels;
				}
			}
			
			double heightCounts = maxCounts - m_backgroundIntercept;
			PeakInfo peakInfo = new PeakInfo(chanOfMaxCounts,
					heightCounts, addWidth511Channels, false,
					m_averagePeakWidthChannels);
			m_peakInfoList.add(peakInfo);
		}
	}
	
	// public methods
	
	public FitInfo clone() {
				
		FitInfo newFitInfo = new FitInfo(m_backgroundIntercept,
				m_backgroundSlope, m_averagePeakWidthChannels,
				m_contains511KeV, m_initialPeakWidthChannels,
				m_peakInfoList);
		
		return newFitInfo;
	}
	
	public double[] getX(final FitVary fitVary) {
		
		int count = fitVary.getVaryCount();
		double[] X = new double[count];
		
		int i = 0;
		
		if (fitVary.bckInterceptVaries()) {
			X[i] = getBckIntercept();
			i++;
		}
		if (fitVary.bckSlopeVaries()) {
			X[i] = getBckSlope();
			i++;
		}
		if (fitVary.avgWidthVaries()) {
			X[i] = getAvgWid();
			i++;
		}
		
		Iterator<PeakInfo> pit = getPeakIterator();
		Iterator<PeakVary> pvt = fitVary.getPeakIterator();
		for ( ; pit.hasNext() && pvt.hasNext(); ) {
			PeakInfo peakInfo = pit.next();
			PeakVary peakVary = pvt.next();
			
			if (peakVary.heightCountsVaries()) {
				X[i] = peakInfo.getHeightCounts();
				i++;
			}
			if (peakVary.centroidChannelsVaries()) {
				X[i] = peakInfo.getCentroidChannels();
				i++;
			}
			if (peakVary.addWidth511Varies()) {
				X[i] = peakInfo.getAddWidth511Channels();
				i++;
			}
		}
		
		return X;
	}
	
	public void update(final RealVector X, final FitVary fitVary) {
		
		int i = 0;
		
		if (fitVary.bckInterceptVaries()) {
			setBckIntercept(X.getEntry(i));
			i++;
		}
		if (fitVary.bckSlopeVaries()) {
			setBckSlope(X.getEntry(i));
			i++;
		}
		if (fitVary.avgWidthVaries()) {
			setAvgWid(X.getEntry(i));
			i++;
		}
		
		Iterator<PeakInfo> pit = getPeakIterator();
		Iterator<PeakVary> pvt = fitVary.getPeakIterator();
		for ( ; pit.hasNext() && pvt.hasNext(); ) {
			PeakInfo peakInfo = pit.next();
			PeakVary peakVary = pvt.next();
			
			if (peakVary.heightCountsVaries()) {
				peakInfo.setHeight(X.getEntry(i));
				i++;
			}
			if (peakVary.centroidChannelsVaries()) {
				peakInfo.setCentroid(X.getEntry(i));
				i++;
			}
			if (peakVary.addWidth511Varies()) {
				peakInfo.setAddWidth511(X.getEntry(i));
				i++;
			}
		}
	}
	
	// package methods
	
	PeakInfo addPeak(double centroidChannels, double centroidEnergy,
			double heightCounts) {
		
		double addWidth511Channels = 0;
		if (!m_contains511KeV) {
			if (isNear511KeV(centroidEnergy)) {
				m_contains511KeV = true;
				addWidth511Channels = m_averagePeakWidthChannels;
			}
		}
		PeakInfo peakInfo = new PeakInfo(centroidChannels, heightCounts,
				addWidth511Channels, false, m_averagePeakWidthChannels);
				
		m_peakInfoList.add(peakInfo);
		
		return peakInfo;
	}
		
	void deletePeak(final PeakInfo peakToDelete) {
		
		m_peakInfoList.remove(peakToDelete);
		// if this was the 511 keV peak, update flag
		if (0 != peakToDelete.getAddWidth511Channels()) {
			m_contains511KeV = false;
		}
	}
	
	double getAvgWid() {
		return m_averagePeakWidthChannels;
	}
	double getBckIntercept() {
		return m_backgroundIntercept;
	}
	double getBckSlope() {
		return m_backgroundSlope;
	}
	double getInitialPeakWidthChannels() {
		return m_initialPeakWidthChannels;
	}
	int getPeakCount() {
		return m_peakInfoList.size();
	}
	Iterator<PeakInfo> getPeakIterator() {
		return m_peakInfoList.iterator();
	}
	
	// private methods

	private static boolean isNear511KeV(double energy) {
		
		boolean answer = false;
		
		if (Math.abs(energy - 511) <= PK511KEV_THRESHOLD) {
			answer = true;
		}
		
		return answer;
	}
	private void setAvgWid(double averagePeakWidthChannels) {
		
		m_averagePeakWidthChannels = averagePeakWidthChannels;
		for (Iterator<PeakInfo> it = getPeakIterator(); it.hasNext(); ) {
			it.next().updateFitInfoAvgWidth(averagePeakWidthChannels);
		}
	}
	private void setBckIntercept(double backgroundIntercept) {
		m_backgroundIntercept = backgroundIntercept;
	}
	private void setBckSlope(double backgroundSlope) {
		m_backgroundSlope = backgroundSlope;
	}
	
	// inner class
	
	/**
	 * holds information about each peak that is used in a region fit
	 *
	 */
	public static class PeakInfo {
		
		// member data
		
		private double       m_heightCounts;
		private double       m_centroidChannels;
		private double       m_addWidth511Channels;
		private boolean      m_fixedCentroid;
		private double       m_fitInfoAvgWidth;
		
		// constructor

		private PeakInfo(double centroidChannels, double heightCounts,
				double addWidth511Channels, boolean fixedCentroid,
				double fitInfoAvgWidth) {
			
			m_heightCounts = heightCounts;
			m_centroidChannels = centroidChannels;
			m_addWidth511Channels = addWidth511Channels;
			m_fixedCentroid = fixedCentroid;		
			m_fitInfoAvgWidth = fitInfoAvgWidth;
		}
		
		// public methods
		
		public PeakInfo clone() {
						
			return new PeakInfo(m_centroidChannels, m_heightCounts,
					m_addWidth511Channels, m_fixedCentroid, m_fitInfoAvgWidth);
		}
		
		// package methods
		
		// used by RegionFitting.checkFit()
		void constrain(ChannelRange region, double initialPeakwidthChannels) {
			
			if ((10 > m_heightCounts) && (0 != m_heightCounts)) {
				m_heightCounts = 10;
			}
			if (0 > m_addWidth511Channels) {
				m_addWidth511Channels = initialPeakwidthChannels;
			}
			if ((region.getFirstChannel() + 2) > m_centroidChannels) {
				m_centroidChannels = region.getFirstChannel() + 2;
			}
			if ((region.getLastChannel() - 2) < m_centroidChannels) {
				m_centroidChannels = region.getLastChannel() - 2;
			}
		}
		
		double getAddWidth511Channels() {
			return m_addWidth511Channels;
		}
		double getArea() {
			return getFwhm() * m_heightCounts * Math.sqrt(AREA_FACTOR_SQUARED);
		}
		double getCentroidChannels() {
			return m_centroidChannels;
		}
		double getFwhm() {
			/* comment from C code:
			 * 10/27/2010
			 * Egger found example of negative peakwidth, but fit is good.
			 * Realize that in constraint code, peakwidth always squared,
			 * so negative is not prevented. Notice also that area() takes
			 * the absolute value of the width plus additions. Do same here.
			 */
			return Math.abs(m_fitInfoAvgWidth + m_addWidth511Channels);
		}
		double getHeightCounts() {
			return m_heightCounts;
		}
		boolean isFixedCentroid() {
			return m_fixedCentroid;
		}
		
		// private methods
		
		private void setAddWidth511(double addWidthChannels) {
			m_addWidth511Channels = addWidthChannels;
		}
		private void setCentroid(double centroidChannels) {
			m_centroidChannels = centroidChannels;
		}
		private void setHeight(double heightCounts) {
			m_heightCounts = heightCounts;
		}
		private void updateFitInfoAvgWidth(double fitInfoAvgWidth) {
			m_fitInfoAvgWidth = fitInfoAvgWidth;
		}
	} // PeakInfo
}