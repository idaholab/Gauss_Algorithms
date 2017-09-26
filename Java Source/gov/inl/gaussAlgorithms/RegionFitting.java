/*
 * Copyright 2017 Battelle Energy Alliance
 */
/*
 *  Gauss Algorithms
 *
 *  File Name: RegionFitting.java
 *
 *  Description: Implements gaussian fit of a region of interest
 */
package gov.inl.gaussAlgorithms;

import java.awt.geom.Point2D;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Set;
import java.util.TreeSet;
import java.util.Vector;

import org.apache.commons.math3.fitting.leastsquares.LeastSquaresOptimizer.Optimum;
import org.apache.commons.math3.fitting.leastsquares.LeastSquaresProblem;
import org.apache.commons.math3.fitting.leastsquares.LevenbergMarquardtOptimizer;
import org.apache.commons.math3.util.Precision;

import gov.inl.gaussAlgorithms.Fit.CycleReturnCode;
import gov.inl.gaussAlgorithms.FitParameters.CCType;
import gov.inl.gaussAlgorithms.FitParameters.PeakWidthMode;
import gov.inl.gaussAlgorithms.FitInfo.PeakInfo;

/**
 * contains the region fitting algorithm
 *
 */
public class RegionFitting {
	
	private final static int ADD_PEAK_DELTA_THRESHOLD = 1;
			
	// constructor
	private RegionFitting() {		
	}
	
	// public methods
	
	public static Vector<Fit> fitRegion(final FitInputs inputs)
	throws Exception {
		
		Vector<Fit> answer = new Vector<Fit>();
		
		TreeSet<Peak> inputPeaks = inputs.getInputPeaks();
		FitParameters parms = inputs.getFitParameters();
		if (inputPeaks.size() > parms.getMaxNpeaks()) {
			throw new Exception("Too many input peaks.");
		}
		
		int maxCycles = parms.getNcycle();
		int[] previousPeakCounts = new int[maxCycles];
		
		Spectrum spectrum = inputs.getSpectrum();
		ChannelRange region = inputs.getRegion();
		EnergyEquation ex = inputs.getEnergyEquation();
		WidthEquation wx = inputs.getWidthEquation();

		FitInfo fitInfo = new FitInfo(spectrum, region, ex, wx, inputPeaks);
		
		ConvergenceCriteria cc = new ConvergenceCriteria(parms);
				
		int cycleNumber = 1;
		FitVary fitVary = new FitVary(parms.getPeakwidthMode(), fitInfo);
		Fit fit = cycle(cycleNumber, inputs, fitInfo, fitVary, cc);
		
		if (null != fit.getCycleException()) {
			throw  fit.getCycleException();
		}
				
		if (fit.getCycleReturnCode() == CycleReturnCode.CONTINUE) {
			answer.add(fit);
			previousPeakCounts[0] = fit.getOutputPeaks().size();
		} else {
			Exception reason = fit.getCycleException();
			if (null == reason) {
				reason = new Exception("Fit failed for unknown reason.");
			}
			throw reason;
		}
				
		double residualThreshold = parms.getMaxResid();
		int maxPeakCount = parms.getMaxNpeaks();

		for (cycleNumber = 2; cycleNumber <= maxCycles; cycleNumber++) {
			int completedCycleCount = cycleNumber - 1;
			int[] peakCountList = new int[completedCycleCount];
			for (int i = 0; i < completedCycleCount; i++) {
				peakCountList[i] = previousPeakCounts[i];
			}
									
			if (CycleReturnCode.DONE == checkFit(fitInfo, peakCountList,
					spectrum, region, fit.getCurve(1), residualThreshold,
					maxPeakCount, ex)) {
				break;
			}
			
			fitVary = new FitVary(parms.getPeakwidthMode(), fitInfo);
			fit = cycle(cycleNumber, inputs, fitInfo, fitVary, cc);
			answer.add(fit);
			previousPeakCounts[cycleNumber - 1] = fit.getOutputPeaks().size();
			if (fit.getCycleReturnCode() != CycleReturnCode.CONTINUE) {
				break;
			}
		}
		
		// Order the results by chi squared, starting with the smallest.
				
		int maxOutCount = parms.getNout();
		
		HashMap<Double, Fit> tempMap = new HashMap<Double, Fit>();
		for (Iterator<Fit> it = answer.iterator(); it.hasNext(); ) {
			Fit fitItem = it.next();
			double chiSquared = fitItem.getChiSquared();
			tempMap.put(new Double(chiSquared), fitItem);
		}
		answer.clear();
		Set<Double> keys = tempMap.keySet();
		TreeSet<Double> orderedKeys = new TreeSet<Double>();
		orderedKeys.addAll(keys);
		int count = 1;
		for (Iterator<Double> it = orderedKeys.iterator(); it.hasNext();
			 count++) {
			Double key = it.next();
			Fit fitItem = tempMap.get(key);
			answer.add(fitItem);
			if (maxOutCount <= count) {
				break;
			}
		}
		
		return answer;
	}
		
	// private methods
	
	private static boolean addPeak(Spectrum spectrum, ChannelRange region,
			FitInfo fitInfo, final Curve curve, double residualThreshold,
			int maxPeakCount, EnergyEquation ex) {
				
		// If there is no room for another peak,
		// don't add a new peak.
		if ((fitInfo.getPeakCount() + 1) > maxPeakCount) {
			return false;
		}

		// find channel with max residual
		
		double maxResidual = 0;
		int channelOfMaxResidual = 0;
		Vector<Point2D.Double> residuals = curve.getResiduals();
		for (Iterator<Point2D.Double> it = residuals.iterator();
			 it.hasNext(); ) {
			Point2D.Double residual = it.next();
			if (maxResidual < residual.getY()) {
				maxResidual = residual.getY();
				channelOfMaxResidual = (int) residual.getX();
			}
		}
		
		// If max residual found is less than threshold,
		// don't add a new peak.
		if (maxResidual < residualThreshold) {
			return false;
		}
		
		// If location is too close to existing peak,
		// don't add a new peak.
		for (Iterator<PeakInfo> it = fitInfo.getPeakIterator();
			 it.hasNext(); ) {
			PeakInfo peakInfo = it.next();
			if (Math.abs(peakInfo.getCentroidChannels() -
					channelOfMaxResidual) < ADD_PEAK_DELTA_THRESHOLD) {
				return false;
			}
		}
		
		if ((channelOfMaxResidual - 1) < region.getFirstChannel()) {
			return false;
		}
		if ((channelOfMaxResidual + 1) >= region.getLastChannel()) {
			return false;
		}
				
		// If neighboring residuals are not greater than zero,
		// don't add a new peak.
		if ((curve.getResidualAt(channelOfMaxResidual - 1).getY() <= 0) &&
			(curve.getResidualAt(channelOfMaxResidual + 1).getY() <= 0)) {
			return false;
		}
				
		// add a new peak
		double fitAtMaxResidual = curve.getFit(channelOfMaxResidual).getY();
		double heightCounts = spectrum.getCountAt(channelOfMaxResidual) -
				fitAtMaxResidual;
		fitInfo.addPeak(channelOfMaxResidual,
				ex.getEnergy(channelOfMaxResidual), heightCounts);
				
		return true;
	}
	
	// this will modify the fit record...
	// potentially adding or deleting a fit peak
	private static CycleReturnCode checkFit(FitInfo fitInfo,
			int[] previousPeakCounts, Spectrum spectrum, ChannelRange region,
			final Curve curve, double residualThreshold, int maxPeakCount,
			EnergyEquation ex) {
		
		boolean lastPeakIsNew = false;
						
		PeakInfo peakToDelete = deletePeak(fitInfo, previousPeakCounts);
		
		if (null != peakToDelete) {
			fitInfo.deletePeak(peakToDelete);
		} else if (! addPeak(spectrum, region, fitInfo, curve,
				residualThreshold, maxPeakCount, ex)) {
			return CycleReturnCode.DONE;
		} else {
			lastPeakIsNew = true;
		}
		
		// constrain other parameters
		// Loop over old remaining peaks.
		// If we just deleted a peak, all remaining get updated.
		// If we just added a peak, all peaks up to the last one get updated.
		
		double initialPeakwidthChannels =
				fitInfo.getInitialPeakWidthChannels();
		for (Iterator<PeakInfo> it = fitInfo.getPeakIterator();
			 it.hasNext(); ) {
			PeakInfo peakInfo = it.next();
			
			if (lastPeakIsNew && !it.hasNext()) {
				break;  // don't update the newly added peak
			}
			
			peakInfo.constrain(region, initialPeakwidthChannels);
		}
		
		return CycleReturnCode.CONTINUE;
	}

	private static Fit cycle(int cycleNumber, final FitInputs inputs,
			final FitInfo fitInfo, FitVary fitVary,
			ConvergenceCriteria cc) {
		
		int denominator = inputs.getRegion().widthChannels() -
				fitVary.getVaryCount();
		if (denominator <= 1) {
			Exception reason =
					new Exception("too many variables. overdefined.");
			return new Fit(cycleNumber, CycleReturnCode.DONE, reason);
		}
		
		Fit results = nllsqs(cycleNumber, inputs, cc, fitInfo, fitVary);
		
		return results;
	}
	
	private static PeakInfo deletePeak(FitInfo fitInfo,
			int[] previousPeakCounts) {
		
		double threshold = .2 * fitInfo.getInitialPeakWidthChannels();
		int currentPeakCount = fitInfo.getPeakCount();
		
		PeakInfo[] peakArray = new PeakInfo[fitInfo.getPeakCount()];
		int ptr = 0;
		for (Iterator<PeakInfo> it = fitInfo.getPeakIterator();
			 it.hasNext(); ptr++) {
			peakArray[ptr] = it.next();
		}
						
		PeakInfo peakToDelete = null;
	
		int topK = fitInfo.getPeakCount();
		int topJ = topK - 1;
		for (int j = 0; j < topJ; j++) {
			
			if (0 != peakArray[j].getHeightCounts()) {
				
				for (int k = j + 1; k < topK; k++) {
					
					// if peaks are too close
				
					if ((0 != peakArray[k].getHeightCounts()) &&
						(Math.abs(peakArray[j].getCentroidChannels() -
						 peakArray[k].getCentroidChannels()) < threshold)) {
						// review npeaks of previous cycles
						// to prevent repeat of fit with same
						// number of components
						
						int completedCycleCount = previousPeakCounts.length;
						if (1 < completedCycleCount) {
							for (int i = 0; i < completedCycleCount; i++) {
								if ((currentPeakCount - 1) ==
									previousPeakCounts[i]) {
									return peakToDelete;
								}
							}
						}
						
						peakToDelete = peakArray[j];
					
						// If height of kth peak is less than that of jth,
						// then delete the kth peak instead.
						// See GAUSS VII manual, section 2.2.5, page 22
						if (peakArray[k].getHeightCounts() <
							peakArray[j].getHeightCounts()) {
							peakToDelete = peakArray[k];
						}
												
						return peakToDelete;
					}
				}
			}
		}
	
		return peakToDelete;
	}
	
	private static Fit nllsqs(int cycleNumber, final FitInputs inputs,
			ConvergenceCriteria cc, final FitInfo fitInfo, FitVary fitVary) {
		
		double ftol = cc.getFtol();
		double xtol = cc.getXtol();
		double gtol = 0;
		
		double initialStepBoundFactor = 100;
		double costRelativeTolerance = ftol;
		double parRelativeTolerance = xtol;
		double orthoTolerance = gtol;
		double qrRankingThreshold = Precision.SAFE_MIN;
		// 7/16/2014 Alan Egger guess is "try zero".
		// 7/16/2014 I just saw default is Precision.SAFE_MIN
		// 8/14/14 try zero to see if residuals will be smaller.
		// it either works or makes no difference... not sure.
		//double qrRankingThreshold = 0;
				
		LevenbergMarquardtOptimizer opt = new LevenbergMarquardtOptimizer(
				initialStepBoundFactor, costRelativeTolerance,
				parRelativeTolerance, orthoTolerance, qrRankingThreshold);
		
		LmderFcn fcn = new LmderFcn(inputs.getSpectrum(), inputs.getRegion(),
				fitInfo, fitVary);
		LeastSquaresProblem problem = fcn.getProblem();
		
		Optimum answer = null;
		try {
			answer = opt.optimize(problem);
		} catch (Exception e) {
			Exception reason = new Exception(
					"Exception in Apache least squares optimizer: " +
					e.getMessage());
			return new Fit(cycleNumber, CycleReturnCode.DONE, reason);
		}

		FitInfo finalFitInfo = fcn.getFinalFitInfo();

		// NOTE: the covariances are calculated by Apache code...		
		// I believe the covariance matrix is pretty standard.
		// old code used covariance tolerance = 0
		FitInfoUncertainty fitInfoUncertainty = new FitInfoUncertainty(
				finalFitInfo, fitVary, answer.getCovariances(0),
				inputs.getEnergyEquation());
		Fit fit = new Fit(inputs, cycleNumber, finalFitInfo, fitVary,
				fitInfoUncertainty, CycleReturnCode.CONTINUE);

		return fit;
	}
	
	// inner classes
	
	// TODO add other lmder settings to this? gtol? qrRankingThreshold?
	private static class ConvergenceCriteria {
		
		// member data
		
		// settings from C code
		//private static double[] m_ftolm = {1.0e-4, 1.0e-3, 1.0e-3, 1.0e-2};
		//private static double[] m_xtolm = {1.0e-4, 1.0e-3, 3.0e-3, 1.0e-2};
		
		// preliminary tests show these work.
		private static double[] m_ftolm = {1.0e-6, 1.0e-5, 1.0e-5, 1.0e-4};
		private static double[] m_xtolm = {1.0e-6, 1.0e-5, 3.0e-5, 1.0e-4};
		private double          m_ftol;
		private double          m_xtol;
		
		ConvergenceCriteria(FitParameters parms) {
			
			m_ftol = 0;
			m_xtol = 0;
			int index = 0;
			
			PeakWidthMode peakWidthMode = parms.getPeakwidthMode();
			CCType cctype = parms.getCcType();
			
			switch (cctype) {
				case LARGER:
					index = 0;
					if (peakWidthMode == PeakWidthMode.VARIES) {
						index = 2;
					} else if (peakWidthMode == PeakWidthMode.FIXED) {
						index = 3;
					}
					break;
				case SMALLER:
					index = 1;
					break;
				case LARGER_INC:
					index = 2;
					break;
				default:
					// leave as zero
					break;
			}
			
			m_ftol = m_ftolm[index];
			m_xtol = m_xtolm[index];
		}
		double getFtol() {
			return m_ftol;
		}
		double getXtol() {
			return m_xtol;
		}
	}
}
