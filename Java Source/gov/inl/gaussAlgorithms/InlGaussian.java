/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: InlGaussian.java
 *
 *  Description: Implements gaussian using old C code. Apache Commons Math
 *               version seems to be incorrect...
 */
package gov.inl.gaussAlgorithms;

import gov.inl.gaussAlgorithms.FitInfo.PeakInfo;

/**
 * this is the gaussian fit that is used
 *
 */
public class InlGaussian {
	
	/**
	 * sqrt(4 * ln(2))
	 */
	public final static double MU_FACTOR = Math.sqrt(4 * Math.log(2));
	private final static int MU_CONSTRAINT = 10;
	
	// member data
	
	private final double             m_mean;
	private final double             m_norm;
	private final double             m_fwhm;
	
	// constructor
		
	public InlGaussian(double norm, double mean, double sigma) {
		
		m_norm = norm;
		m_mean = mean;
		m_fwhm = sigma * MU_FACTOR * Math.sqrt(2);
	}
	
	public InlGaussian(PeakInfo peakInfo) {
		
		m_norm = peakInfo.getHeightCounts();
		m_mean = peakInfo.getCentroidChannels();
		m_fwhm = peakInfo.getFwhm();
	}
	
	
	/* This does not provide same curve that InlGaussian does.
	 * Why?
	 * Is it because InlGaussian.valueConstrained() is always called?
	 * 
	public static Gaussian getGaussian(PeakInfo peakInfo) {
		
		return new Gaussian(peakInfo.getHeightCounts(),
				peakInfo.getCentroidChannels(),
				peakInfo.getFwhm() / (MU_FACTOR * Math.sqrt(2)));
	}
	 */

	
	// public methods
	
	public static double expNegMuSquared(double mu) {
		
		return Math.exp(-(mu * mu));
	}
	
	public double mu(double x) {
		
		return mu(x, m_mean, m_fwhm);
	}
		
	public static double muConstrained(double mu) {
				
		double muConstrained = mu;
		
		if (MU_CONSTRAINT < muConstrained) {
			muConstrained = MU_CONSTRAINT;
		} else if (-MU_CONSTRAINT > muConstrained) {
			muConstrained = -MU_CONSTRAINT;
		}
		
		return muConstrained;
	}	

	/* currently unused
	public double value(double x) {
		
		double mu = mu(x, m_mean, m_fwhm);
		double eNegMuCnSqr = expNegMuSquared(mu);
		double gaussian = m_norm * eNegMuCnSqr;
		
		return gaussian;
	} */
	
	public double valueConstrained(double x) {
		
		double mu = mu(x, m_mean, m_fwhm);
		double muConstrained = muConstrained(mu);
		double eNegMuCnSqr = expNegMuSquared(muConstrained);
		double gaussian = m_norm * eNegMuCnSqr;
		
		return gaussian;
	}
	
	// private methods
	
	private static double mu(double x, double mean, double fwhm) {
		
		double mu = 0;
		if (0 != fwhm) {
			mu = (x - mean) * MU_FACTOR / fwhm;
		}
		
		return mu;
	}	
}
