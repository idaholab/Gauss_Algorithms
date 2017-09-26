/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: CalibPair.java
 *
 *  Description: used to match a value (energy or peakwidth) with a channel
 */
package gov.inl.gaussAlgorithms;

/**
 * An X-Y pair used to calibrate energy or peak width with a least-squares fit.
 *
 */
public class CalibPair implements Comparable<Object> {
	
	protected static final float THRESHOLD = (float) .00001;

	// member data
	
	protected double             m_centroid;
	protected double             m_value;
	protected double             m_valueUncertainty;
	
	public CalibPair(double centroid, double value) {
		
		m_centroid = centroid;
		m_value = value;
		m_valueUncertainty = 1;  // use for unweighted calibration
	}
	
	public CalibPair(double centroid, double value,
			double valueUncertainty) {
		
		m_centroid = centroid;
		m_value = value;
		m_valueUncertainty = valueUncertainty;
	}
	
	// public methods

	public int compareTo(Object otherObject) {
		
		if (!(otherObject instanceof CalibPair)) {
			return -1;
		}
		
		CalibPair otherPair = (CalibPair) otherObject;
		
		int answer = compareDouble(this.m_centroid, otherPair.m_centroid);
		if (0 == answer) {
			answer = compareDouble(this.m_value, otherPair.m_value);
		}
		
		// ignore uncertainty
		
		return answer;
	}

	public boolean equals(Object otherObject) {
						
		if (0 == compareTo(otherObject)) {
			return true;
		}
		
		return false;
	}
	
	public double getCentroid() {
		return m_centroid;
	}
	
	public double getValue() {
		return m_value;
	}
	
	public double getValueUncertainty() {
		return m_valueUncertainty;
	}
	
	public int hashCode() {
		
		String hashString = Double.toString(m_centroid) +
							Double.toString(m_value);

		// ignore uncertainty

		return hashString.hashCode();
	}
	
	// private methods

	private int compareDouble(double fromThisPair, double fromOtherPair) {

		int answer = 0;

		double diff = Math.abs(fromThisPair - fromOtherPair);
		if (diff > THRESHOLD) {
			answer = Double.compare(fromThisPair, fromOtherPair);
		}

		return answer;
	}
}
