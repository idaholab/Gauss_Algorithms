/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: EnergyCalibrating.java
 *
 *  Description: Implements methods for calibrating energy from pairs.
 */
package gov.inl.gaussAlgorithms;

import java.util.TreeSet;

/**
 * a collection of methods for calibrating energy from channel-energy pairs
 *
 */
public class EnergyCalibrating {
	
	// constructor
	private EnergyCalibrating() {
		// singleton. only provides static methods.
	}

	// public static methods
	
	public static EnergyEquation calibrate(final double[] channel,
			final double[] energy, final double[] sige,
			EnergyEquation.MODE egyEqnMode, boolean weighted)
	throws Exception {
		
		TreeSet<CalibPair> pairs = new TreeSet<CalibPair>();
		
		int pairCount = Math.min(channel.length, energy.length);
		pairCount = Math.min(pairCount, sige.length);
		
		for (int i = 0; i < pairCount; i++) {
			double weight = 1;
			if (weighted) {
				weight = sige[i];
			}
			CalibPair pair = new CalibPair(channel[i], energy[i], weight);
			pairs.add(pair);
		}
		
		return calibrate(pairs, egyEqnMode);
	}
	
	public static EnergyEquation calibrate(final TreeSet<CalibPair> pairs,
			EnergyEquation.MODE egyEqnMode)
	throws Exception {
		
		int coefficientCount = 3;
		if (EnergyEquation.MODE.LINEAR.equals(egyEqnMode)) {
			coefficientCount = 2;
		}
		double[] coeffs = LeastSquareFitting.linf(pairs, coefficientCount);
		
		double quadCoeff = 0;
		double chiSquared = coeffs[2];
		if (EnergyEquation.MODE.QUADRATIC.equals(egyEqnMode)) {
			quadCoeff = coeffs[2];
			chiSquared = coeffs[3];
		}
		EnergyEquation ex = new EnergyEquation(coeffs[0], coeffs[1], quadCoeff,
				chiSquared, egyEqnMode);
		
		return ex;
	}
}
