/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: LmderFcn.java
 *
 *  Description: Implements "fcn" for use by lmder
 */
package gov.inl.gaussAlgorithms;

import gov.inl.gaussAlgorithms.FitInfo.PeakInfo;
import gov.inl.gaussAlgorithms.FitVary.PeakVary;

import java.awt.geom.Point2D;
//import java.io.File;
//import java.text.NumberFormat;
import java.util.Iterator;
import java.util.Vector;

import org.apache.commons.math3.fitting.leastsquares.LeastSquaresFactory;
import org.apache.commons.math3.fitting.leastsquares.LeastSquaresProblem;
import org.apache.commons.math3.fitting.leastsquares.MultivariateJacobianFunction;
import org.apache.commons.math3.linear.Array2DRowRealMatrix;
import org.apache.commons.math3.linear.ArrayRealVector;
import org.apache.commons.math3.linear.RealMatrix;
import org.apache.commons.math3.linear.RealVector;
import org.apache.commons.math3.optim.ConvergenceChecker;
import org.apache.commons.math3.util.Pair;

/**
 * the famous "lmder" (Levenberg-Marquardt) fit used to fit regions
 *
 */
public class LmderFcn {

	// member data
	
	private final Spectrum              m_spectrum;
	private final ChannelRange          m_region;
	private FitInfo                     m_fitInfo;
	private final FitVary               m_fitVary;
	private LeastSquaresProblem         m_problem;
	
	// constructor
		
	public LmderFcn(Spectrum spectrum, ChannelRange region,
			final FitInfo fitInfo, FitVary fitVary) {
		
		m_spectrum = spectrum;
		m_region = region;
		m_fitInfo = fitInfo.clone();
		m_fitVary = fitVary;
		
		double[] target = getTarget(region);
		RealVector observations = new ArrayRealVector(target);
		double[] X = fitInfo.getX(fitVary);
		RealVector start = new ArrayRealVector(X);
		int maxEval = 100 * (X.length + 1);
		int maxIter = maxEval;
		MultivariateJacobianFunction model = new FcnJacobian();
		ConvergenceChecker<LeastSquaresProblem.Evaluation> checker = null;
		
		m_problem = LeastSquaresFactory.create(model, observations, start,
				checker, maxEval, maxIter);
	}
	
	public FitInfo getFinalFitInfo() {
		return m_fitInfo;
	}
	
	public LeastSquaresProblem getProblem() {
		return m_problem;
	}
		
	// private methods
	
	private static double[] getTarget(final ChannelRange region) {
		
		double[] target = new double[region.widthChannels()];
		
		int top = region.getLastChannel();
		for (int i = 0, chan = region.getFirstChannel(); chan < top;
			 i++, chan++) {
			target[i] = 0;
		}
		
		return target;
	}
	
	// inner classes
	
	private class FcnJacobian implements MultivariateJacobianFunction {
		
		// constructor
		private FcnJacobian() {
			
		}
		
		// public methods

		public Pair<RealVector, RealMatrix> value(RealVector currentX) {
						
			m_fitInfo.update(currentX, m_fitVary);

			Pair<RealVector, RealMatrix> answer =
					new Pair<RealVector, RealMatrix>(getFx(),
							getJacobian());
			
			return answer;
		}
		
		// private methods
		
		private RealVector getFx() {
			
			// NOTE: target is all zeros
						
			Curve curve = new Curve(m_spectrum, m_region, 1, m_fitInfo);
			
			Vector<Point2D.Double> residPoints = curve.getResiduals();
			double[] resid = new double[residPoints.size()];
			int i = 0;
			for (Iterator<Point2D.Double> it = residPoints.iterator();
				 it.hasNext(); i++) {
				Point2D.Double point = it.next();
				resid[i] = point.getY();
			}
			ArrayRealVector fx = new ArrayRealVector(resid);
						
			return fx;
		}
		
		private RealMatrix getJacobian() {
									
			//  see output.c:GP_calc_jacobian
			
			int rowDimension = m_region.widthChannels();
			int colDimension = m_fitVary.getVaryCount();
			RealMatrix jacobian = new Array2DRowRealMatrix(
					rowDimension, colDimension);
						
			// loop over columns
			int colIndex = 0;
			
			if (m_fitVary.bckInterceptVaries()) {
				// make a vector of all 1's and copy to matrix
				// after dividing by negative spectrum uncertainty
				double[] column = new double[rowDimension];
				int chan = m_region.getFirstChannel();
				for (int i = 0; i < rowDimension; i++, chan++) {
					column[i] = 1;
					column[i] = - column[i] / m_spectrum.getSigCountAt(chan);
				}
				jacobian.setColumn(colIndex, column);
				colIndex++;
			}
			if (m_fitVary.bckSlopeVaries()) {
				// make an array of row indices and copy to matrix
				// after dividing by negative spectrum uncertainty
				double[] column = new double[rowDimension];
				int chan = m_region.getFirstChannel();
				for (int i = 0; i < rowDimension; i++, chan++) {
					column[i] = i;
					column[i] = - column[i] / m_spectrum.getSigCountAt(chan);
				}
				jacobian.setColumn(colIndex, column);
				colIndex++;
			}
			if (m_fitVary.avgWidthVaries()) {
				// set each row to sum of "2*y*muSquared/fwhm" of each peak
				// divided by negative spectrum uncertainty
				double[] column = new double[rowDimension];
				int chan = m_region.getFirstChannel();
				for (int i = 0; i < rowDimension; i++, chan++) {
					column[i] = 0;
					Iterator<PeakInfo> pit = m_fitInfo.getPeakIterator();
					for ( ; pit.hasNext(); ) {
						PeakInfo peakInfo = pit.next();
						InlGaussian inlGaussian = new InlGaussian(peakInfo);
						double gaussian = inlGaussian.valueConstrained(chan);
						double mu = inlGaussian.mu(chan);
						double addWidth511 = 0;
						double fwhm = peakInfo.getFwhm();
						if (0 != fwhm) {
							addWidth511 = 2 * gaussian * mu * mu / fwhm;
						}
						column[i] += addWidth511;
					}
					column[i] = - column[i] / m_spectrum.getSigCountAt(chan);
				}
				jacobian.setColumn(colIndex, column);
				colIndex++;
				
			}

			Iterator<PeakInfo> pit = m_fitInfo.getPeakIterator();
			Iterator<PeakVary> pvt = m_fitVary.getPeakIterator();
			for ( ; pit.hasNext() && pvt.hasNext(); ) {
				PeakInfo peakInfo = pit.next();
				PeakVary peakVary = pvt.next();
				
				InlGaussian inlGaussian = new InlGaussian(peakInfo);

				if (peakVary.heightCountsVaries()) {
					
					// set each row to exp(-muSquared)
					// divided by negative spectrum uncertainty
					double[] column = new double[rowDimension];
					int chan = m_region.getFirstChannel();
					for (int i = 0; i < rowDimension; i++, chan++) {
						double mu = inlGaussian.mu(chan);
						double muConstrained = InlGaussian.muConstrained(mu);
						column[i] = InlGaussian.expNegMuSquared(muConstrained);
						
						column[i] = -  column[i]/
								m_spectrum.getSigCountAt(chan);
					}
					jacobian.setColumn(colIndex, column);
					colIndex++;
				}
				if (peakVary.centroidChannelsVaries()) {
					// set each row to 2*y*mu*factor / fwhm
					// divided by negative spectrum uncertainty
					double[] column = new double[rowDimension];
					int chan = m_region.getFirstChannel();
					for (int i = 0; i < rowDimension; i++, chan++) {
						double gaussian = inlGaussian.valueConstrained(chan);
						double mu = inlGaussian.mu(chan);
						double centroid = 0;
						double fwhm = peakInfo.getFwhm();
						if (0 != fwhm) {
							centroid = 2.0 * gaussian * mu *
									InlGaussian.MU_FACTOR / fwhm;
						}
						column[i] = centroid;
						column[i] = - column[i] /
								m_spectrum.getSigCountAt(chan);
					}
					jacobian.setColumn(colIndex, column);
					colIndex++;
				}
				if (peakVary.addWidth511Varies()) {
					// set each row to 2*y*muSquared / fwhm
					// divided by negative spectrum uncertainty
					double[] column = new double[rowDimension];
					int chan = m_region.getFirstChannel();
					for (int i = 0; i < rowDimension; i++, chan++) {
						double gaussian = inlGaussian.valueConstrained(chan);
						double mu = inlGaussian.mu(chan);
						double addWidth511 = 0;
						double fwhm = peakInfo.getFwhm();
						if (0 != fwhm) {
							addWidth511 = 2 * gaussian * mu * mu / fwhm;
						}
						column[i] = - addWidth511 /
								m_spectrum.getSigCountAt(chan);
					}
					jacobian.setColumn(colIndex, column);
					colIndex++;
				}
			}
									
			return jacobian;
		}
	} // FcnJacobian
}
