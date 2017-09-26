/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: Fit.java
 *
 *  Description: contains the complete result of a region fit
 */
package gov.inl.gaussAlgorithms;

import gov.inl.gaussAlgorithms.Summary.PeakSummary;

import java.awt.geom.Point2D;
import java.text.NumberFormat;
import java.util.Iterator;
import java.util.TreeSet;
import java.util.Vector;

/**
 * contains all inputs and results of a region fit
 *
 */
public class Fit {
	
	/**
	 * this enumerates the answers that could be returned from any given fit cycle
	 *
	 */
	public static enum CycleReturnCode {DONE, DELETE, ADD, CONTINUE};

	// member data

	private final FitInputs           m_inputs;
	private final int                 m_cycleNumber;
	private final double              m_chiSq;
	private final CycleReturnCode     m_cycleReturnCode; // new with java work
	private final Exception           m_cycleException; // new with java work
	private final FitInfo             m_fitInfo;
	private final BackgroundEquation  m_background;
	private Curve                     m_curve;
	private final Summary             m_summary;
	
	// constructors
	
	// to be used by RegionFitting java code on failure
	Fit(int cycleNumber, CycleReturnCode cycleReturnCode, Exception e) {
		
		m_cycleNumber = cycleNumber;
		m_cycleReturnCode = cycleReturnCode;
		m_cycleException = e;
				
		m_inputs = null;
		m_fitInfo = null;
		m_background = null;
		m_curve = null;
		m_summary = null;
		m_chiSq = 0;
	}
	
	Fit(final FitInputs inputs, int cycleNumber,
			final FitInfo fitInfo, FitVary fitVary,
			final FitInfoUncertainty fitInfoUncertainty,
			CycleReturnCode cycleReturnCode) {
				
		m_cycleNumber = cycleNumber;
		m_cycleReturnCode = cycleReturnCode;
		m_cycleException = null;
		
		m_inputs = inputs;
		m_fitInfo = fitInfo;
		m_background = new BackgroundEquation(fitInfo, fitInfoUncertainty);
		
		Spectrum spectrum = inputs.getSpectrum();
		ChannelRange region = inputs.getRegion();
		m_curve = new Curve(spectrum, region, 1, fitInfo);
		m_chiSq = calculateChiSq(m_curve, fitVary.getVaryCount());
		EnergyEquation usedEx = inputs.getEnergyEquation();
		TreeSet<Peak> inputPeaks = inputs.getInputPeaks();
		m_summary = new Summary(spectrum, region, usedEx, inputPeaks,
				fitInfo, fitInfoUncertainty);
	}
	
	// public methods
	
	public BackgroundEquation getBackground() {
		return m_background;
	}
		
	public double getChiSquared() {
		
		return m_chiSq;
	}

	public Curve getCurve(int nPlotsPerChannel) {
		
		if (nPlotsPerChannel != m_curve.getNPlotsPerChannel()) {
			m_curve = new Curve(m_inputs.getSpectrum(), m_inputs.getRegion(),
					nPlotsPerChannel, m_fitInfo);
		}
		
		return m_curve;
	}
	
	public Exception getCycleException() {
		return m_cycleException;
	}
	
	public int getCycleNumber() {
		return m_cycleNumber;
	}
	
	public CycleReturnCode getCycleReturnCode() {
		return m_cycleReturnCode;
	}
	
	public EnergyEquation getEnergyEquation() {
		return m_inputs.getEnergyEquation();
	}
	
	public FitParameters getFitParameters() {
		return m_inputs.getFitParameters();
	}
	
	public TreeSet<Peak> getInputPeaks() {
		return m_inputs.getInputPeaks();
	}
	
	public TreeSet<Peak> getOutputPeaks() {
		
		TreeSet<Peak> peaks = new TreeSet<Peak>();

		for (Iterator<PeakSummary> it =
				m_summary.getPeakSummaries().iterator(); it.hasNext(); ) {
			PeakSummary peakSummary = it.next();
			
			boolean fixed = false;
			if (Peak.THRESHOLD >
				Math.abs(peakSummary.getChannelUncertainty())) {
				fixed = true;
			}
			Peak peak = new Peak(peakSummary.getChannel(), fixed);
			peak.update(m_inputs.getEnergyEquation());
			peaks.add(peak);
		}

		return peaks;
	}
	
	public String getPlainText(final NumberFormat ofMaxFractionDigits,
			String lineSeparator) {

		StringBuffer buf = new StringBuffer(80);

		Vector<String> lines = getPlainLines(ofMaxFractionDigits);
		for (Iterator<String> it = lines.iterator(); it.hasNext(); ) {
			buf.append(it.next() + lineSeparator);
		}

		String plainText = buf.toString();
		return plainText;
	}

	public ChannelRange getRegion() {
		return m_inputs.getRegion();
	}
	
	public Spectrum getSpectrum() {
		return m_inputs.getSpectrum();
	}

	public Summary getSummary() {
		return m_summary;
	}
	
	public String getToolTipText(final NumberFormat ofMaxFractionDigits) {

		Vector<String> lines = getPlainLines(ofMaxFractionDigits);

		StringBuffer buf = new StringBuffer(80);
		
		Iterator<String> it = lines.iterator();

		// construct first line
		buf.append("<html>");
		if (it.hasNext()) {
			buf.append(it.next());
		}

		// construct middle lines
		while (it.hasNext()) {
			buf.append("<br>" + it.next());
		}

		// end the last line
		buf.append("</html>");

		String toolTipText = buf.toString();

		return toolTipText;
	}

	public WidthEquation getWidthEquation() {
		return m_inputs.getWidthEquation();
	}
		
	// private methods

	private static double calculateChiSq(final Curve curve, int varyCount) {
		
		Vector<Point2D.Double> residuals = curve.getResiduals();
				
		double sumSquares = 0;
		for (Iterator<Point2D.Double> it = residuals.iterator();
			 it.hasNext(); ) {
			Point2D.Double point = it.next();
			sumSquares += Math.pow(point.getY(), 2);
		}
		
		double denominator = residuals.size() - varyCount;
		
		return sumSquares / denominator;
	}
	
	private Vector<String> getPlainLines(final NumberFormat ofMaxFractionDigits) {

		Vector<String> lines = new Vector<String>();

		lines.add(m_inputs.getRegion().toString());
		lines.add(m_inputs.getEnergyEquation().toString());
		lines.add(m_inputs.getWidthEquation().toString());
		FitParameters parms = m_inputs.getFitParameters();
		lines.add("Max Cycles=" + parms.getNcycle() +
				  "; Max Fits Returned=" + parms.getNout() +
				  "; Max Npeaks=" + parms.getMaxNpeaks() +
				  "; Max Residual=" + parms.getMaxResid());
		
		String pw = parms.getPeakwidthMode().name();
		String cc = parms.getCcType().displayLabel();
		lines.add("Peak Width=" + pw + "; Convergence Criteria=" + cc);

		NumberFormat format = ofMaxFractionDigits;
		int nPeaks = m_summary.getPeakSummaries().size();
		lines.add("Fit Cycle=" + m_cycleNumber +
				  "; Cycle returned=" + m_cycleReturnCode +
				  "; Fit Chi Squared=" + format.format(m_chiSq) +
				  "; Area Ratio=" + format.format(m_summary.getRatio()) +
				  "; NPeaks=" + nPeaks);

		lines.add("Peak Record: chan, sigc, height, sigh, " +
				  "width, sigw, area, siga, energy, sige");
		int i = 0;
		for (Iterator<PeakSummary> it =
				m_summary.getPeakSummaries().iterator(); it.hasNext(); i++) {
			PeakSummary peakSummary = it.next();
			lines.add("Peak " + (i+1) + ": " +
					format.format(peakSummary.getChannel()) + ", " +
					format.format(peakSummary.getChannelUncertainty()) + ", " +
					format.format(peakSummary.getHeight()) + ", " +
					format.format(peakSummary.getHeightUncertainty()) + ", " +
					format.format(peakSummary.getWidth()) + ", " +
					format.format(peakSummary.getWidthUncertainty()) + ", " +
					format.format(peakSummary.getArea()) + ", " +
					format.format(peakSummary.getAreaUncertainty()) + ", " +
					format.format(peakSummary.getEnergy()) + ", " +
					format.format(peakSummary.getEnergyUncertainty()));
		}

		lines.add("background: intercept at start of region,  slope");
		lines.add("background: " +
				   format.format(m_fitInfo.getBckIntercept()) + ", " +
				   format.format(m_fitInfo.getBckSlope()));

		return lines;
	}
}
