/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: Curve.java
 *
 *  Description: implements a fit curve that can be graphed
 */
package gov.inl.gaussAlgorithms;

import gov.inl.gaussAlgorithms.FitInfo.PeakInfo;

import java.awt.geom.Point2D;
import java.util.Iterator;
import java.util.Vector;

/**
 * This contains the graph of a gaussian fit of a region.
 */
public class Curve {

	private final ChannelRange                     m_region;
	private final int                              m_nPeaks;
	private final int                              m_nPlotsPerChannel;
	private final int                              m_plotCount;
	private final Vector<Point2D.Double>           m_backgroundCurve;
	private final Vector<Vector<Point2D.Double>>   m_peakCurves;
	private final Vector<Point2D.Double>           m_fitCurveAllPlots;
	private final Vector<Point2D.Double>           m_fitsAtChannels;
	private final Vector<Point2D.Double>           m_chanResiduals;


	public Curve(Spectrum spectrum, ChannelRange region, int nPlotsPerChan,
			final FitInfo fitInfo) {
		
		m_region = region;
		m_nPeaks = fitInfo.getPeakCount();
		m_nPlotsPerChannel = nPlotsPerChan;
		
		m_plotCount = getPlotCount(region, nPlotsPerChan);
		m_backgroundCurve = constructFitBackgroundCurve(region, nPlotsPerChan,
				fitInfo.getBckSlope(), fitInfo.getBckIntercept());

		
		m_peakCurves = new Vector<Vector<Point2D.Double>>();
		for (Iterator<PeakInfo> it = fitInfo.getPeakIterator();
			 it.hasNext(); ) {
			m_peakCurves.add(constructPeakCurve(m_backgroundCurve, it.next()));
		}
		
		m_fitCurveAllPlots = constructFitCurve(m_nPlotsPerChannel,
				m_backgroundCurve, m_peakCurves);
		
		m_fitsAtChannels = constructFitsAtChannels(region, fitInfo);
		
		m_chanResiduals = constructResiduals(spectrum, m_fitsAtChannels);
	}

	public Vector<Point2D.Double> getBackPoints() {

		return m_backgroundCurve;
	}

	public Vector<Point2D.Double> getComponentPoints(int peakIndex) {

		return m_peakCurves.get(peakIndex);
	}

	public Vector<Point2D.Double> getCurvePoints() {

		return m_fitCurveAllPlots;
	}
	
	public Point2D.Double getFit(int channel) {
		
		int i = channel - m_region.getFirstChannel();
		
		return m_fitsAtChannels.get(i);
	}
	
	public double getMaxY(int channel, int channelDelta) {
		
		double maxY = 0;
		
		int startChannel = m_region.getFirstChannel();
		int endChannel = m_region.getLastChannel();
		
		if ((channel < startChannel) || (channel > endChannel)) {
			return maxY;
		}
		
		int firstChannel = channel - channelDelta;
		firstChannel = Math.max(firstChannel, startChannel);
		int lastChannel = channel + channelDelta;
		lastChannel = Math.min(lastChannel, endChannel);
		
		// find max in overall fit curve
		for (Iterator<Point2D.Double> it = m_fitCurveAllPlots.iterator();
			 it.hasNext(); ) {
			Point2D.Double fitPoint = it.next();
			double currentChan = fitPoint.getX();
			if (currentChan < firstChannel) {
				continue;
			}
			if (currentChan > lastChannel) {
				break;
			}
			maxY = Math.max(maxY, fitPoint.getY());
		}

		// compare against max in each component fit
		for (Iterator<Vector<Point2D.Double>> itPeak =
				m_peakCurves.iterator(); itPeak.hasNext(); ) {
			Vector<Point2D.Double> componentCurve = itPeak.next();
			
			for (Iterator<Point2D.Double> itPoint = componentCurve.iterator();
				 itPoint.hasNext(); ) {
				Point2D.Double compPoint = itPoint.next();
				double currentChan = compPoint.getX();
				if (currentChan < firstChannel) {
					continue;
				}
				if (currentChan > lastChannel) {
					break;
				}
				maxY = Math.max(maxY, compPoint.getY());
			}
		}
				
		return maxY;
	}

	public int getNpeaks() {
		return m_nPeaks;
	}
	
	public int getNPlotsPerChannel() {
		return m_nPlotsPerChannel;
	}

	public int getNumPlottedPoints() {
		return m_plotCount;
	}
	
	public Point2D.Double getResidualAt(int channel) {
		
		return m_chanResiduals.get(channel - m_region.getFirstChannel());
	}
	
	public Vector<Point2D.Double> getResiduals() {
		return m_chanResiduals;
	}
	
	// private methods
	
	private static Vector<Point2D.Double> constructFitBackgroundCurve(
			ChannelRange region, int nPlotsPerChannel, double slope,
			double intercept) {
		
		Vector<Point2D.Double> points = new Vector<Point2D.Double>();
		
	    int plotCount = getPlotCount(region, nPlotsPerChannel);
	    double plotDelta = 1.0 / (double) nPlotsPerChannel;
	    
	    double xOffset = 0;
	    double x = region.getFirstChannel();
	    for (int i = 0; i < plotCount;
	    	 i++, xOffset += plotDelta, x += plotDelta) {
	    	double y = intercept + (slope * xOffset);
	    	points.add(new Point2D.Double(x, y));
	    }
	    
	    return points;
	}
	
	private static Vector<Point2D.Double> constructFitCurve(
			int nPlotsPerChannel,
			final Vector<Point2D.Double> backgroundPoints,
			final Vector<Vector<Point2D.Double>> peakCurves) {
				
		Vector<Point2D.Double> points = new Vector<Point2D.Double>();

		int plotCount = backgroundPoints.size();
		for (int i = 0; i < plotCount; i++) {
			Point2D.Double backgroundPoint = backgroundPoints.get(i);
			
    		double x = backgroundPoint.getX();
    		double backgroundY = backgroundPoint.getY();
    		double y = backgroundY;
			for (Iterator<Vector<Point2D.Double>> it = peakCurves.iterator();
				 it.hasNext(); ) {
				// take background back out of each component
				y += it.next().get(i).getY() - backgroundY;
			}
			
			Point2D.Double newPoint = new Point2D.Double(x, y);
			points.add(newPoint);						
		}
		
		return points;
	}
	
	private static Vector<Point2D.Double> constructFitsAtChannels(
			ChannelRange region, final FitInfo fitInfo) {
			
		Vector<Point2D.Double> backgroundCurve = constructFitBackgroundCurve(
				region, 1, fitInfo.getBckSlope(), fitInfo.getBckIntercept());

		Vector<Vector<Point2D.Double>> peakCurves =
				new Vector<Vector<Point2D.Double>>();
		for (Iterator<PeakInfo> it = fitInfo.getPeakIterator();
			 it.hasNext(); ) {
			peakCurves.add(constructPeakCurve(backgroundCurve, it.next()));
		}
		
		Vector<Point2D.Double> fitsAtChannels = constructFitCurve(
				1, backgroundCurve, peakCurves);
				
		return fitsAtChannels;
	}
	
	private static Vector<Point2D.Double> constructPeakCurve(
			final Vector<Point2D.Double> backgroundPoints,
			final PeakInfo peakInfo) {
		
		Vector<Point2D.Double> points = new Vector<Point2D.Double>();
    	
		InlGaussian inlGaussian = new InlGaussian(peakInfo);

    	int plotCount = backgroundPoints.size();
    	for (int i = 0; i < plotCount; i++) {
    		Point2D.Double backgroundPoint = backgroundPoints.get(i);
    		double x = backgroundPoint.getX();
    		double y = inlGaussian.valueConstrained(x);
    		y += backgroundPoint.getY();
    		points.add(new Point2D.Double(x, y));
    	}

    	return points;
	}
	
	private static Vector<Point2D.Double> constructResiduals(Spectrum spectrum,
			final Vector<Point2D.Double> fitsAtChannels) {
				
		Vector<Point2D.Double> residuals = new Vector<Point2D.Double>();
				
		for (Iterator<Point2D.Double> it = fitsAtChannels.iterator();
			 it.hasNext(); ) {
			Point2D.Double fitPoint = it.next();
			int channel = (int) fitPoint.getX();
			double count = spectrum.getCountAt(channel);
			double sigCount = spectrum.getSigCountAt(channel);
			double resid = 0;
			if (0 != sigCount) {
				resid = (count - fitPoint.getY()) / sigCount;
			}
			residuals.add(new Point2D.Double(channel, resid));
		}
						
		return residuals;
	}

	private static int getPlotCount(ChannelRange region,
			int nPlotsPerChannel) {
		
		int plotCount = region.getLastChannel() - region.getFirstChannel();
		plotCount *= nPlotsPerChannel; // for the first "n-1" channels
		plotCount++; // for the last channel
		
		return plotCount;
	}	
}
