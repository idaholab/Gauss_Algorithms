/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: LeastSquareFitting
 *
 *  Description: Implements linear least squares fit of x,y pairs
 *               with optional uncertainties
 */
package gov.inl.gaussAlgorithms;

import java.util.Iterator;
import java.util.TreeSet;

/**
 * the least square fit implementation used to calibrate energy and width
 *
 */
public class LeastSquareFitting {

	// constructor
	private LeastSquareFitting() {
	}
	
	// public methods
	
	/*
	 * linf	 routine to do a linear least squares fit of
	 *		 a list of (value, centroid) coordinates.
	 *       (with centroid uncertainties)
	 *       (fitting a polynomial to a list of points)
	 *       
	 *       The list of coefficients, followed by the chi-squared
	 *       is returned in the array of doubles.
	 */
	public static double[] linf(final TreeSet<CalibPair> pairs,
			int coefficientCount) {
		
		double[] sp = new double[coefficientCount];
		double[][] a = new double[coefficientCount][coefficientCount];
		double[][] ai = new double[coefficientCount][coefficientCount];
		
		// set up matrix and vector
		
		for (int i = 0; i < coefficientCount; i++) {
			for (int j = 0; j < coefficientCount; j++) {
				int mp = i + j;
				sp[j] = 0;
				a[i][j] = 0;
				for (Iterator<CalibPair> it = pairs.iterator();
					 it.hasNext(); ) {
					CalibPair pair = it.next();
					double denominator = pair.getValueUncertainty();
					denominator = denominator * denominator;
					sp[j] = sp[j] +
						(Math.pow(pair.getCentroid(), j) * pair.getValue()) /
						denominator;
					a[i][j] = a[i][j] +
						Math.pow(pair.getCentroid(), mp) / denominator;
				}
			}
		}
		
		// solve matrix equations
		
		for (int i = 0; i < coefficientCount; i++) {
			for (int j = 0; j < coefficientCount; j++) {
				double temp = a[i][i] * a[j][j];
				if (temp <= 0) {
					return null;
				}
				
				ai[i][j] = a[i][j] / Math.sqrt(temp);
			}
		}
		
		double det = matinv(ai);
		
		// if singular matrix, return error
		if (0 == det) {
			return null;
		}
		
		for (int i = 0; i < coefficientCount; i++) {
			for (int j = 0; j < coefficientCount; j++) {
				double temp = a[i][i] * a[j][j];
				if (temp <= 0) {
					return null;
				}
				
				ai[i][j] = ai[i][j] / Math.sqrt(temp);
			}
		}
		
		// find solution
		
		double[] polycoeffs = new double[coefficientCount + 1];
		
		for (int i = 0; i < coefficientCount; i++) {
			polycoeffs[i] = 0;
			for (int j = 0; j < coefficientCount; j++) {
				polycoeffs[i] = polycoeffs[i] + (ai[i][j] * sp[j]);
			}
		}
		
		// don't calculate chiSquared if pairCount == coefficientCount
		
		double chiSquared = 0;
		int pairCount = pairs.size();
		
		if (pairCount != coefficientCount) {
			for (Iterator<CalibPair> it = pairs.iterator(); it.hasNext(); ) {
				CalibPair pair = it.next();
				double calcY = 0;
				for (int i = 0; i < coefficientCount; i++) {
					calcY = calcY +
						(polycoeffs[i] * Math.pow(pair.getCentroid(), i));
				}
				
				double temp = (pair.getValue() - calcY) /
					pair.getValueUncertainty();
				chiSquared = chiSquared + (temp * temp);
			}
			
			chiSquared = chiSquared / (pairCount - coefficientCount);
		}
		
		polycoeffs[coefficientCount] = chiSquared;
		
		return polycoeffs;
	}

	// private methods
	
	/*
	 * findMax	find and return the largest value
	 *		in a 2 dimensional array beyond the first k rows and
	 *		columns that is also greater than or equal to the amax
	 *		value passed in. Return max found along with its location.
	 */
	private static AmaxIkJk findMax(int k, final double[][] array,
			double amax) {
		
		AmaxIkJk answer = null;
		double maxFound = amax;

		int order = array[0].length;
		for (int i = k; i < order; i++) {
			for (int j = k; j < order; j++) {
				if (Math.abs(maxFound) <= Math.abs(array[i][j])) {
					answer = new AmaxIkJk(array[i][j], i, j);
					maxFound = answer.getAmax();
				}
			}
		}
				
		return answer;
	}

	/*
	 * matinv	invert a symmetric matrix and calculate its determinant.
	 *          The input matrix is replaced by its inverse.
	 *          The determinant is returned.
	 *
	 * routine taken from Bevington's "Data Reduction and Error Analysis
	 * for the Physical Sciences" -- see Gauss-Jordan elimination
	 */
	private static double matinv(double[][] array) {
		
		int order = array[0].length;
		int[] ik = new int[order];
		int[] jk = new int[order];		
		for (int row = 0; row < order; row++) {
			ik[row] = 0;
			jk[row] = 0;
		}
		
		double det = 1;
		for (int k = 0; k < order; k++) {
			double localAmax = 0;
			
			AmaxIkJk swapAnswer = swapColumns(k, array, localAmax);
			if (swapAnswer == null) {
				return 0;
			}
			localAmax = swapAnswer.getAmax();
			ik[k] = swapAnswer.getIk();
			jk[k] = swapAnswer.getJk();

			// interchange rows to put amax in array [k][k]

			while (jk[k] < k) {
				swapAnswer = swapColumns(k, array, localAmax);
				if (swapAnswer == null) {
					return 0;
				}
				localAmax = swapAnswer.getAmax();
				ik[k] = swapAnswer.getIk();
				jk[k] = swapAnswer.getJk();
			}
		
			if (jk[k] > k) {
				int j = jk[k];
				for (int i = 0; i < order; i++) {
					double save = array[i][k];
					array[i][k] = array[i][j];
					array[i][j] = -save;
				}
			}

			// accumulate elements of inverse matrix

			for (int i = 0; i < order; i++) {
				if (i != k) {
					array[i][k] = - array[i][k] / localAmax;
				}
			}
			
			for (int i = 0; i < order; i++) {
				for (int j = 0; j < order; j++) {
					if ((i != k) && (j != k)) {
						array[i][j] = array[i][j] +
										(array[i][k] * array[k][j]);
					}
				}
			}

			for (int j = 0; j < order; j++) {
				if (j != k) {
					array[k][j] = array[k][j] / localAmax;
				}
			}	            
			
			array[k][k] = 1.0 / localAmax;
			det = det * localAmax;
		}
		
		// restore ordering of matrix
		
		for (int k = order - 1; k >= 0; k--) {
			int j = ik[k];
			
			if (j > k) {
				for (int i = 0; i < order; i++) {
					double save = array[i][k];
					array[i][k] = - array[i][j];
					array[i][j] = save;
				}
			}
			
			int i = jk[k];
			if (i > k) {
				for (j = 0; j < order; j++) {
					double save = array[k][j];
					array[k][j] = - array[i][j];
					array[i][j] = save;
				}
			}
		}
				
		return det;
	}

	/*
	 * swapColumns	called from matinv() to rearrange matrix entries.
	 */
	private static AmaxIkJk swapColumns(int k, double[][] array,
			double amax) {
				
		double localAmax = amax;
		AmaxIkJk foundMax = findMax(k, array, localAmax);
		if (null == foundMax) {
			return null;
		}
		int ik = foundMax.getIk();
		localAmax = foundMax.getAmax();
		
		// interchange columns to put amax in array[k][k]

		//
		// observation:  if order, k, and array are not changed, and
		//               amax was the value returned from the last call,
		//               I expect CA_find_max() to return the same values
		//               for amax, ik, and jk again...
		// question:     so why is it being called again?
		//

		while (ik < k) {
			foundMax = findMax(k, array, localAmax);
			if (null == foundMax) {
				return null;
			}
			ik = foundMax.getIk();
			localAmax = foundMax.getAmax();
		}
						
		int order = array[0].length;
		if (ik > k) {
			int i = ik;
			for (int j = 0; j < order; j++) {
				// swap array[k][j] with array[ik][j]

				double save = array[k][j];
				array[k][j] = array[i][j];
				array[i][j] = -save;				
			}
		}
					
		return foundMax;
	}
	
	private static class AmaxIkJk {
		
		// member data
		private double        m_amax;
		private int           m_ik;
		private int           m_jk;
		
		// constructor
		AmaxIkJk(double amax, int ik, int jk) {
			m_amax = amax;
			m_ik = ik;
			m_jk = jk;
		}
		
		// package methods
		double getAmax() {
			return m_amax;
		}
		int getIk() {
			return m_ik;
		}
		int getJk() {
			return m_jk;
		}
	}
}
