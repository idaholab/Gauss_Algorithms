/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: FitInputs.java
 *
 *  Description: holds fit inputs: parameters, peaks, region, calibration,
 *               and spectrum
 */
package gov.inl.gaussAlgorithms;

import java.util.Iterator;
import java.util.TreeSet;

/**
 * holds all of the inputs for a region fit
 *
 */
public class FitInputs {
	
	// member data
	
	private final Spectrum         m_spectrum;
	private final EnergyEquation   m_ex;
	private final WidthEquation    m_wx;
	private final ChannelRange     m_region;
	private final TreeSet<Peak>    m_inputPeaks;
	private final FitParameters    m_parms;
	
	// constructor
	
	public FitInputs(final Spectrum spectrum, final EnergyEquation ex,
			final WidthEquation wx, final ChannelRange region,
			final TreeSet<Peak> inputPeaks, final FitParameters parms) {
		
		m_spectrum = spectrum;
		m_ex = ex;
		m_wx = wx;
		m_region = region;
		
		// protect against setting changes
		
		m_parms = (FitParameters) parms.clone();
		
		m_inputPeaks = new TreeSet<Peak>();
		
		for (Iterator<Peak> it = inputPeaks.iterator(); it.hasNext(); ) {
			Peak newPeak = (Peak) it.next().clone();
			m_inputPeaks.add(newPeak);
		}
	}
	
	// public methods

	public EnergyEquation getEnergyEquation() {
		return m_ex;
	}
	
	public FitParameters getFitParameters() {
		return m_parms;
	}
	
	public TreeSet<Peak> getInputPeaks() {
		return m_inputPeaks;
	}
	
	public ChannelRange getRegion() {
		return m_region;
	}
	
	public Spectrum getSpectrum() {
		return m_spectrum;
	}
	
	public WidthEquation getWidthEquation() {
		return m_wx;
	}
}
