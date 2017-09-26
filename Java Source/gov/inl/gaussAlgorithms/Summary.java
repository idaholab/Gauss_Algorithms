/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: Summary.java
 *
 *  Description: Implements a summary of a region fit
 */
package gov.inl.gaussAlgorithms;

import gov.inl.gaussAlgorithms.FitInfo.PeakInfo;
import gov.inl.gaussAlgorithms.FitInfoUncertainty.PeakInfoUncertainty;

import java.util.Iterator;
import java.util.TreeSet;
import java.util.Vector;

/**
 * some information extracted from a region fit result
 *
 */
public class Summary {
	
	// to match input peaks to summary peaks
	private final static double  CENTROID_THRESHOLD = .00001;

	// ratio of summation area to integral area
	private double                      m_ratio;
	private TreeSet<PeakSummary>        m_peakSummaries;
	
	public Summary(Spectrum spectrum, ChannelRange region, EnergyEquation ex,
			final TreeSet<Peak> inputPeaks, final FitInfo fitInfo,
			final FitInfoUncertainty fitInfoUncertainty) {
				
		m_peakSummaries = new TreeSet<PeakSummary>();
				
		Iterator<PeakInfo> pit = fitInfo.getPeakIterator();
		Iterator<PeakInfoUncertainty> put =
				fitInfoUncertainty.getPeakIterator();
		for ( ; pit.hasNext() && put.hasNext(); ) {
			PeakInfo peakInfo = pit.next();
			PeakInfoUncertainty peakInfoUncertainty = put.next();
			
			double channel = peakInfo.getCentroidChannels();
			double sigc = peakInfoUncertainty.getCentroidChannelsUncert();
			double width = peakInfo.getFwhm();
			double sigw = peakInfoUncertainty.getFwhmUncert();
			double height = peakInfo.getHeightCounts();
			double sigh = peakInfoUncertainty.getHeightCountsUncert();
			double area = peakInfo.getArea();
			double siga = peakInfoUncertainty.getAreaUncert();
			double energy = ex.getEnergy(channel);
			double sige = peakInfoUncertainty.getCentroidEnergyUncert(ex);
			PeakSummary summary = new PeakSummary(channel, sigc, height,
					sigh, width, sigw, area, siga, energy, sige);
			m_peakSummaries.add(summary);
		}
		
		// look for fixed centroids
		for (Peak peak: inputPeaks) {
			
			if (peak.isCentroidFixed()) {
				for (PeakSummary summary: m_peakSummaries) {
					
					if (Math.abs(summary.getChannel() - peak.getChannel())
							< CENTROID_THRESHOLD) {
						summary.fixChannel(true);
						break;
					}
				}
			}
		}
		
		// look for centroids outside region
		
		for (PeakSummary peakSummary: m_peakSummaries) {
			if (!region.contains(peakSummary.getChannel())) {
				peakSummary.setIsOutside(true);
			}
		}
		
		// look for +/- peak pairs
		// Look for any peak pair within fwhm/2 that have opposite heights.
		Vector<PeakSummary> negPeaks = new Vector<PeakSummary>();
		Vector<PeakSummary> posPeaks = new Vector<PeakSummary>();
			
		for (PeakSummary peakSummary: m_peakSummaries) {
					
			if (0 > peakSummary.getHeight()) {
				negPeaks.add(peakSummary);
			} else if (0 < peakSummary.getHeight()){
				posPeaks.add(peakSummary);
			}
		}
		
		double halfWidth = fitInfo.getAvgWid() / 2.0;

		for (PeakSummary negPeak: negPeaks) {
			for (PeakSummary posPeak: posPeaks) {
				
				if (halfWidth > Math.abs(
						negPeak.getChannel() - posPeak.getChannel())) {
					negPeak.setOfPosNegPair(true);
					posPeak.setOfPosNegPair(true);
				}
			}
		}
		
		// set the ratio
		m_ratio = calculateRatio(spectrum,
				region, fitInfo, m_peakSummaries);
	}

	public TreeSet<PeakSummary> getPeakSummaries() {
		return m_peakSummaries;
	}

	public double getRatio() {
		return m_ratio;
	}
	
	// private methods
		
	private double calculateRatio(Spectrum spectrum,
			ChannelRange region, FitInfo fitInfo,
			final TreeSet<PeakSummary> peakSummaries) {
		
		double averageBackground = fitInfo.getBckIntercept() +
				(fitInfo.getBckSlope() *
					(region.getLastChannel() - region.getFirstChannel()) / 2);
		
		double areaSummation = 0;
		int top = region.getLastChannel();
		for (int i = region.getFirstChannel(); i <= top; i++) {
			areaSummation += spectrum.getCountAt(i) - averageBackground;
		}
		
		double integralSummation = 0;
		for (Iterator<PeakSummary> it = peakSummaries.iterator();
			 it.hasNext(); ) {
			PeakSummary peakSummary = it.next();
			integralSummation += peakSummary.getArea();
		}
		
		double ratio = 0;
		if (0 != integralSummation) {
			ratio = areaSummation / integralSummation;
		}
		
		return ratio;
	}
		
	// inner classes
	
	/**
	 * some information about each peak extracted from a region fit result
	 *
	 */
	public static class PeakSummary implements Comparable<PeakSummary> {

		private final double   m_channel;
		private final double   m_sigc;
		private boolean        m_channelFixed;
		private final double   m_height;
		private final double   m_sigh;
		private final double   m_width;
		private final double   m_sigw;
		private final double   m_area;
		private final double   m_siga;
		private final double   m_energy;
		private final double   m_sige;
		private final boolean  m_isNegPeak;
		private boolean        m_isOutsidePeak;
		private boolean        m_ofPosNegPair;

		private PeakSummary(double channel, double sigc, double height,
				double sigh, double width, double sigw, double area,
				double siga, double energy, double sige) {
			
			m_channel = channel;
			m_sigc = sigc;
			m_channelFixed = false;
			m_height = height;
			m_sigh = sigh;
			m_width = width;
			m_sigw = sigw;
			m_area = area;
			m_siga = siga;
			m_energy = energy;
			m_sige = sige;
			m_isNegPeak = checkNegPeak(height);
			m_isOutsidePeak = false;
			m_ofPosNegPair = false;
		}
		// public methods
		public int compareTo(PeakSummary other) {
			
			// order by channel
			Double thisChannel = new Double(m_channel);
			Double otherChannel = new Double(other.m_channel);
			
			return thisChannel.compareTo(otherChannel);
		}
		public double getArea() {
			return m_area;
		}
		public double getAreaUncertainty() {
			return m_siga;
		}
		public double getChannel() {
			return m_channel;
		}
		public double getChannelUncertainty() {
			return m_sigc;
		}
		public double getEnergy() {
			return m_energy;
		}
		public double getEnergyUncertainty() {
			return m_sige;
		}
		public double getHeight() {
			return m_height;
		}
		public double getHeightUncertainty() {
			return m_sigh;
		}
		public double getWidth() {
			return m_width;
		}
		public double getWidthUncertainty() {
			return m_sigw;
		}
		public boolean isChannelFixed() {
			return m_channelFixed;
		}
		public boolean isNegPeak() {
			return m_isNegPeak;
		}
		public boolean isOutsidePeak() {
			return m_isOutsidePeak;
		}
		public boolean ofPosNegPair() {
			return this.m_ofPosNegPair;
		}
		// private methods
		private boolean checkNegPeak(double height) {
			if (height < 0) {
				return true;
			}
			return false;
		}
		private void fixChannel(boolean fixChannel) {
			m_channelFixed = fixChannel;
		}
		private void setIsOutside(boolean isOutside) {
			m_isOutsidePeak = isOutside;
		}
		private void setOfPosNegPair(boolean ofPair) {
			m_ofPosNegPair = ofPair;
		}

	}
}
