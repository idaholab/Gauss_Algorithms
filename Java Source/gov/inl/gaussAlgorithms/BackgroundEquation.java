/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: BackgroundEquation.java
 *
 *  Description: contains the background equation of a region fit
 */
package gov.inl.gaussAlgorithms;

/**
 * Linear function of channel describing the background of a gaussian fit of a region.
 *
 */
public class BackgroundEquation {
	
	// member data
	
	private final double       m_intercept;
	private final double       m_interceptUncert;
	private final double       m_slope;
	private final double       m_slopeUncert;
	
	// constructor
	
	/**
	 * Returns a BackgroundEquation object extracted from a gaussian fit.
	 * The equation is a function of channel in the form:
	 *   "b(x) = slope * x + y-intercept"
	 * @param fitInfo        information from a gaussian fit
	 * @param fitInfoUncert  corresponding uncertainty of the fit
	 */
	public BackgroundEquation(FitInfo fitInfo,
			                  FitInfoUncertainty fitInfoUncert) {
		
		m_intercept = fitInfo.getBckIntercept();
		m_interceptUncert = fitInfoUncert.getBckInterceptUncert();
		m_slope = fitInfo.getBckSlope();
		m_slopeUncert = fitInfoUncert.getBckSlopeUncert();
	}
	
	// public methods
	
	/**
	 * Returns the fit background (Y) at the specified channel (X)
	 * @param channel  X
	 * @return         Y
	 */
	public double getBackground(double channel) {
		return m_intercept + (m_slope * channel);
	}
	
	/**
	 * Returns the Y intercept of the background equation.
	 * @return b(0)
	 */
	public double getIntercept() {
		return m_intercept;
	}
	
	/**
	 * Returns the uncertainty of the Y intercept.
	 * @return uncertainty of b(0)
	 */
	public double getInterceptUncert() {
		return m_interceptUncert;
	}
	
	/**
	 * Returns the coefficient of the linear term of the equation.
	 * @return the slope
	 */
	public double getSlope() {
		return m_slope;
	}
	
	/**
	 * Returns the uncertainty of the coefficient of the linear term.
	 * @return the slope uncertainty
	 */
	public double getSlopeUncert() {
		return m_slopeUncert;
	}
}
