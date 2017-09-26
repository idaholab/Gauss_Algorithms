/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: WidthCalibrating.java
 *
 *  Description: Implements methods for calibrating peak width from pairs.
 */
package gov.inl.gaussAlgorithms;

import java.util.Iterator;
import java.util.TreeSet;

/**
 * a collection of methods for calibrating peak width from channel-width pairs
 *
 */
public class WidthCalibrating {

	// constructor
	private WidthCalibrating() {
		// singleton. only provides static methods.
	}
	
	// public static methods
	
	public static WidthEquation calibrate(final double[] channel,
			final double[] peakWidth, final double[] sigw,
			WidthEquation.MODE widEqnMode, boolean weighted)
	throws Exception {

		TreeSet<CalibPair> pairs = new TreeSet<CalibPair>();
		
		int pairCount = Math.min(channel.length, peakWidth.length);
		pairCount = Math.min(pairCount, sigw.length);
		
		for (int i = 0; i < pairCount; i++) {
			double weight = 1;
			if (weighted) {
				weight = sigw[i] * 2 * peakWidth[i];
			}
			CalibPair pair = new CalibPair(channel[i], peakWidth[i], weight);
			pairs.add(pair);
		}

		return calibrate(pairs, widEqnMode);
	}
	
	// QUESTION: can I trust user to set up weight correctly here? see above
	public static WidthEquation calibrate(final TreeSet<CalibPair> pairs,
			WidthEquation.MODE widEqnMode)
	throws Exception {
				
		TreeSet<CalibPair> squaredPairs = new TreeSet<CalibPair>();
		
		for (Iterator<CalibPair> it = pairs.iterator(); it.hasNext(); ) {
			CalibPair pair = it.next();
			double value = pair.getValue();
			if (WidthEquation.MODE.SQUARE_ROOT.equals(widEqnMode)) {
				value = value * value;
			}
			CalibPair squaredPair = new CalibPair(pair.getCentroid(),
					value, pair.getValueUncertainty());
			squaredPairs.add(squaredPair);
		}

		double[] coeffs = LeastSquareFitting.linf(squaredPairs, 2);
		
		WidthEquation wx = new WidthEquation(coeffs[0], coeffs[1], coeffs[2],
				widEqnMode);
		
		return wx;
	}
}
