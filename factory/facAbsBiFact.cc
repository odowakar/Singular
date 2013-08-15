/*****************************************************************************\
 * Computer Algebra System SINGULAR
\*****************************************************************************/
/** @file facAbsBiFact.cc
 *
 * @author Martin Lee
 *
 **/
/*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "timing.h"
#include "debug.h"

#include "facAbsBiFact.h"
#include "facBivar.h"
#include "facFqBivar.h"
#include "cf_reval.h"
#include "cf_primes.h"
#include "cf_algorithm.h"
#ifdef HAVE_FLINT
#include "FLINTconvert.h"
#include <flint/fmpz_poly_factor.h>
#endif
#ifdef HAVE_NTL
#include "NTLconvert.h"
#include <NTL/LLL.h>
#endif

#ifdef HAVE_NTL

TIMING_DEFINE_PRINT(fac_Qa_factorize)
TIMING_DEFINE_PRINT(fac_evalpoint)

CFAFList uniAbsFactorize (const CanonicalForm& F)
{
  CFFList rationalFactors= factorize (F);
  CFFListIterator i= rationalFactors;
  CanonicalForm LcF= rationalFactors.getFirst().factor();
  i++;
  Variable alpha;
  CFAFList result;
  CFFList QaFactors;
  CFFListIterator iter;
  for (; i.hasItem(); i++)
  {
    if (degree (i.getItem().factor()) == 1)
    {
      result.append (CFAFactor (i.getItem().factor(), 1, i.getItem().exp()));
      continue;
    }
    alpha= rootOf (i.getItem().factor());
    QaFactors= factorize (i.getItem().factor(), alpha);
    iter= QaFactors;
    if (iter.getItem().factor().inCoeffDomain())
    {
      LcF *= iter.getItem().factor();
      iter++;
    }
    for (;iter.hasItem(); iter++)
    {
      if (degree (iter.getItem().factor()) == 1)
      {
        result.append (CFAFactor (iter.getItem().factor(), getMipo (alpha),
                                  i.getItem().exp()));
        break;
      }
    }
  }
  result.insert (CFAFactor (LcF, 1, 1));
  return result;
}


/// absolute factorization of bivariate poly over Q
///
/// @return absFactorize returns a list whose entries contain three entities:
///         an absolute irreducible factor, an irreducible univariate polynomial
///         that defines the minimal field extension over which the irreducible
///         factor is defined and the multiplicity of the absolute irreducible
///         factor
CFAFList absBiFactorize (const CanonicalForm& G ///<[in] bivariate poly over Q
                        )
{
  //TODO handle homogeneous input
  ASSERT (getNumVars (G) <= 2, "expected bivariate input");
  ASSERT (getCharacteristic() == 0, "expected poly over Q");

  CFMap N;
  CanonicalForm F= compress (G, N);
  bool isRat= isOn (SW_RATIONAL);
  if (isRat)
    F *= bCommonDen (F);

  Off (SW_RATIONAL);
  F /= icontent (F);
  if (isRat)
    On (SW_RATIONAL);

  CanonicalForm contentX= content (F, 1);
  CanonicalForm contentY= content (F, 2);
  F /= (contentX*contentY);
  CFAFList contentXFactors, contentYFactors;
  contentXFactors= uniAbsFactorize (contentX);
  contentYFactors= uniAbsFactorize (contentY);

  if (contentXFactors.getFirst().factor().inCoeffDomain())
    contentXFactors.removeFirst();
  if (contentYFactors.getFirst().factor().inCoeffDomain())
    contentYFactors.removeFirst();
  if (F.inCoeffDomain())
  {
    CFAFList result;
    for (CFAFListIterator i= contentXFactors; i.hasItem(); i++)
      result.append (CFAFactor (N (i.getItem().factor()), i.getItem().minpoly(),
                                i.getItem().exp()));
    for (CFAFListIterator i= contentYFactors; i.hasItem(); i++)
      result.append (CFAFactor (N (i.getItem().factor()),i.getItem().minpoly(),
                                i.getItem().exp()));
    normalize (result);
    result.insert (CFAFactor (Lc (G), 1, 1));
    return result;
  }
  CFFList rationalFactors= factorize (F);

  CFAFList result, resultBuf;

  CFAFListIterator iter;
  CFFListIterator i= rationalFactors;
  i++;
  for (; i.hasItem(); i++)
  {
    resultBuf= absBiFactorizeMain (i.getItem().factor());
    for (iter= resultBuf; iter.hasItem(); iter++)
      iter.getItem()= CFAFactor (iter.getItem().factor(),
                                 iter.getItem().minpoly(), i.getItem().exp());
    result= Union (result, resultBuf);
  }

  for (CFAFListIterator i= result; i.hasItem(); i++)
    i.getItem()= CFAFactor (N (i.getItem().factor()), i.getItem().minpoly(),
                            i.getItem().exp());
  for (CFAFListIterator i= contentXFactors; i.hasItem(); i++)
    result.append (CFAFactor (N(i.getItem().factor()), i.getItem().minpoly(),
                              i.getItem().exp()));
  for (CFAFListIterator i= contentYFactors; i.hasItem(); i++)
    result.append (CFAFactor (N(i.getItem().factor()), i.getItem().minpoly(),
                              i.getItem().exp()));
  normalize (result);
  result.insert (CFAFactor (Lc(G), 1, 1));

  return result;
}

//TODO optimize choice of p -> choose p as large as possible (better than small p since factorization mod p does not require field extension, also less lifting)
int
choosePoint (const CanonicalForm& F, int tdegF, CFArray& eval, bool rec,
             int absValue)
{
  REvaluation E1 (1, 1, IntRandom (absValue));
  REvaluation E2 (2, 2, IntRandom (absValue));
  if (rec)
  {
    E1.nextpoint();
    E2.nextpoint();
  }

  CanonicalForm f, f1, f2, Fp;
  int i, p;
  CFFList f1Factors, f2Factors;
  CFFListIterator iter;
  int count= 0;
  while (1)
  {
    count++;
    f1= E1 (F);
    if (!f1.isZero() && degree (f1) == degree (F,2))
    {
      f1Factors= factorize (f1);
      if (f1Factors.getFirst().factor().inCoeffDomain())
        f1Factors.removeFirst();
      if (f1Factors.length() == 1 && f1Factors.getFirst().exp() == 1)
      {
        f= E2(f1);
        f2= E2 (F);
        f2Factors= factorize (f2);
        Off (SW_RATIONAL);
        if (f2Factors.getFirst().factor().inCoeffDomain())
          f2Factors.removeFirst();
        if (f2Factors.length() == 1 && f2Factors.getFirst().exp() == 1)
        {
          ZZX NTLf1= convertFacCF2NTLZZX (f1);
          ZZX NTLf2= convertFacCF2NTLZZX (f2);
          ZZ NTLD1= discriminant (NTLf1);
          ZZ NTLD2= discriminant (NTLf2);
          CanonicalForm D1= convertZZ2CF (NTLD1);
          CanonicalForm D2= convertZZ2CF (NTLD2);
          if ((!f.isZero()) &&
              (abs(f)>cf_getSmallPrime (cf_getNumSmallPrimes()-1)))
          {
            for (i= cf_getNumPrimes()-1; i >= 0; i--)
            {
              if (f % CanonicalForm (cf_getPrime (i)) == 0)
              {
                p= cf_getPrime(i);
                Fp= mod (F,p);
                if (totaldegree (Fp) == tdegF &&
                    degree (mod (f2,p), 1) == degree (F,1) &&
                    degree (mod (f1, p),2) == degree (F,2))
                {
                  if (mod (D1, p) != 0 && mod (D2, p) != 0)
                  {
                    eval[0]= E1[1];
                    eval[1]= E2[2];
                    return p;
                  }
                }
              }
            }
          }
          else if (!f.isZero())
          {
            for (i= cf_getNumSmallPrimes()-1; i >= 0; i--)
            {
              if (f % CanonicalForm (cf_getSmallPrime (i)) == 0)
              {
                p= cf_getSmallPrime (i);
                Fp= mod (F,p);
                if (totaldegree (Fp) == tdegF &&
                    degree (mod (f2, p),1) == degree (F,1) &&
                    degree (mod (f1,p),2) == degree (F,2))
                {
                  if (mod (D1, p) != 0 && mod (D2, p) != 0)
                  {
                    eval[0]= E1[1];
                    eval[1]= E2[2];
                    return p;
                  }
                }
              }
            }
          }
        }
        E2.nextpoint();
        On (SW_RATIONAL);
      }
    }
    E1.nextpoint();
    if (count == 2)
    {
      count= 0;
      absValue++;
      E1=REvaluation (1, 1, IntRandom (absValue));
      E2=REvaluation (2, 2, IntRandom (absValue));
      E1.nextpoint();
      E2.nextpoint();
    }
  }
  return 0;
}

//G is assumed to be bivariate, irreducible over Q, primitive wrt x and y, compressed
CFAFList absBiFactorizeMain (const CanonicalForm& G, bool full)
{
  CanonicalForm F= bCommonDen (G)*G;
  bool isRat= isOn (SW_RATIONAL);
  Off (SW_RATIONAL);
  F /= icontent (F);
  On (SW_RATIONAL);
  int minTdeg, tdegF= totaldegree (F);
  CanonicalForm Fp, smallestFactor;
  int p;
  CFFList factors;
  Variable alpha;
  bool rec= false;
  Variable x= Variable (1);
  Variable y= Variable (2);
  CanonicalForm bufF= F;
  CFFListIterator iter;
  CFArray eval= CFArray (2);
  int absValue= 1;
differentevalpoint:
  while (1)
  {
    TIMING_START (fac_evalpoint);
    p= choosePoint (F, tdegF, eval, rec, absValue);
    TIMING_END_AND_PRINT (fac_evalpoint, "time to find eval point: ");

    //after here isOn (SW_RATIONAL)==false
    setCharacteristic (p);
    Fp=F.mapinto();
    factors= factorize (Fp);

    if (factors.getFirst().factor().inCoeffDomain())
      factors.removeFirst();

    if (factors.length() == 1 && factors.getFirst().exp() == 1)
    {
      if (absIrredTest (Fp))
      {
        if (isRat)
          On (SW_RATIONAL);
        setCharacteristic(0);
        return CFAFList (CFAFactor (G, 1, 1));
      }
      else
      {
        setCharacteristic (0);
        if (modularIrredTestWithShift (F))
        {
          if (isRat)
            On (SW_RATIONAL);
          return CFAFList (CFAFactor (G, 1, 1));
        }
        rec= true;
        continue;
      }
    }
    iter= factors;
    smallestFactor= iter.getItem().factor();
    while (smallestFactor.isUnivariate() && iter.hasItem())
    {
      iter++;
      if (!iter.hasItem())
        break;
      smallestFactor= iter.getItem().factor();
    }

    minTdeg= totaldegree (smallestFactor);
    if (iter.hasItem())
      iter++;
    for (; iter.hasItem(); iter++)
    {
      if (!iter.getItem().factor().isUnivariate() &&
          (totaldegree (iter.getItem().factor()) < minTdeg))
      {
        smallestFactor= iter.getItem().factor();
        minTdeg= totaldegree (smallestFactor);
      }
    }
    if (tdegF % minTdeg == 0)
      break;
    setCharacteristic(0);
    rec=true;
  }
  CanonicalForm Gp= Fp/smallestFactor;
  Gp= Gp /Lc (Gp);

  CanonicalForm Gpy= Gp (eval[0].mapinto(), 1);
  CanonicalForm smallestFactorEvaly= smallestFactor (eval[0].mapinto(),1);
  CanonicalForm Gpx= Gp (eval[1].mapinto(), 2);
  CanonicalForm smallestFactorEvalx= smallestFactor (eval[1].mapinto(),2);

  bool xValid= !(Gpx.inCoeffDomain() || smallestFactorEvalx.inCoeffDomain() ||
               !gcd (Gpx, smallestFactorEvalx).inCoeffDomain());
  bool yValid= !(Gpy.inCoeffDomain() || smallestFactorEvaly.inCoeffDomain() ||
               !gcd (Gpy, smallestFactorEvaly).inCoeffDomain());
  if (!xValid || !yValid)
  {
    rec= true;
    setCharacteristic (0);
    goto differentevalpoint;
  }

  setCharacteristic (0);

  CanonicalForm mipo;

  CFArray mipos= CFArray (2);
  CFFList mipoFactors;
  for (int i= 1; i < 3; i++)
  {
    CanonicalForm Fi= F(eval[i-1],i);

    int s= tdegF/minTdeg + 1;
    CanonicalForm bound= power (maxNorm (Fi), 2*(s-1));
    bound *= power (CanonicalForm (s),s-1);
    bound *= power (CanonicalForm (2), ((s-1)*(s-1))/2); //possible int overflow

    CanonicalForm B = p;
    long k = 1L;
    while ( B < bound ) {
      B *= p;
      k++;
    }

    //TODO take floor (log2(k))
    k= k+1;
#ifdef HAVE_FLINT
    fmpz_poly_t FLINTFi;
    convertFacCF2Fmpz_poly_t (FLINTFi, Fi);
    setCharacteristic (p);
    nmod_poly_t FLINTFpi, FLINTGpi;
    if (i == 2)
    {
      convertFacCF2nmod_poly_t (FLINTFpi,
                                smallestFactorEvalx/lc (smallestFactorEvalx));
      convertFacCF2nmod_poly_t (FLINTGpi, Gpx/lc (Gpx));
    }
    else
    {
      convertFacCF2nmod_poly_t (FLINTFpi,
                                smallestFactorEvaly/lc (smallestFactorEvaly));
      convertFacCF2nmod_poly_t (FLINTGpi, Gpy/lc (Gpy));
    }
    nmod_poly_factor_t nmodFactors;
    nmod_poly_factor_init (nmodFactors);
    nmod_poly_factor_insert (nmodFactors, FLINTFpi, 1L);
    nmod_poly_factor_insert (nmodFactors, FLINTGpi, 1L);

    long * link= new long [2];
    fmpz_poly_t *v= new fmpz_poly_t[2];
    fmpz_poly_t *w= new fmpz_poly_t[2];
    fmpz_poly_init(v[0]);
    fmpz_poly_init(v[1]);
    fmpz_poly_init(w[0]);
    fmpz_poly_init(w[1]);

    fmpz_poly_factor_t liftedFactors;
    fmpz_poly_factor_init (liftedFactors);
    _fmpz_poly_hensel_start_lift (liftedFactors, link, v, w, FLINTFi,
                                  nmodFactors, k);

    nmod_poly_factor_clear (nmodFactors);
    nmod_poly_clear (FLINTFpi);
    nmod_poly_clear (FLINTGpi);

    setCharacteristic(0);
    CanonicalForm liftedSmallestFactor=
    convertFmpz_poly_t2FacCF ((fmpz_poly_t &)liftedFactors->p[0],x);

    CanonicalForm otherFactor=
    convertFmpz_poly_t2FacCF ((fmpz_poly_t &)liftedFactors->p[1],x);
    modpk pk= modpk (p, k);
#else
    modpk pk= modpk (p, k);
    ZZX NTLFi=convertFacCF2NTLZZX (pk (Fi*pk.inverse (lc(Fi))));
    setCharacteristic (p);

    if (fac_NTL_char != p)
    {
      fac_NTL_char= p;
      zz_p::init (p);
    }
    zz_pX NTLFpi, NTLGpi;
    if (i == 2)
    {
      NTLFpi=convertFacCF2NTLzzpX (smallestFactorEvalx/lc(smallestFactorEvalx));
      NTLGpi=convertFacCF2NTLzzpX (Gpx/lc (Gpx));
    }
    else
    {
      NTLFpi=convertFacCF2NTLzzpX (smallestFactorEvaly/lc(smallestFactorEvaly));
      NTLGpi=convertFacCF2NTLzzpX (Gpy/lc (Gpy));
    }
    vec_zz_pX modFactors;
    modFactors.SetLength(2);
    modFactors[0]= NTLFpi;
    modFactors[1]= NTLGpi;
    vec_ZZX liftedFactors;
    MultiLift (liftedFactors, modFactors, NTLFi, (long) k);
    setCharacteristic(0);
    CanonicalForm liftedSmallestFactor=
                  convertNTLZZX2CF (liftedFactors[0], x);

    CanonicalForm otherFactor=
                  convertNTLZZX2CF (liftedFactors[1], x);
#endif

    Off (SW_SYMMETRIC_FF);
    liftedSmallestFactor= pk (liftedSmallestFactor);
    if (i == 2)
      liftedSmallestFactor= liftedSmallestFactor (eval[0],1);
    else
      liftedSmallestFactor= liftedSmallestFactor (eval[1],1);

    On (SW_SYMMETRIC_FF);
    CFMatrix M= CFMatrix (s, s);
    M(s,s)= power (CanonicalForm (p), k);
    for (int j= 1; j < s; j++)
    {
      M (j,j)= 1;
      M (j+1,j)= -liftedSmallestFactor;
    }

    mat_ZZ NTLM= *convertFacCFMatrix2NTLmat_ZZ (M);

    ZZ det;

    transpose (NTLM, NTLM);
    (void) LLL (det, NTLM, 1L, 1L); //use floating point LLL ?
    transpose (NTLM, NTLM);
    M= *convertNTLmat_ZZ2FacCFMatrix (NTLM);

    mipo= 0;
    for (int j= 1; j <= s; j++)
      mipo += M (j,1)*power (x,s-j);

    mipoFactors= factorize (mipo);
    if (mipoFactors.getFirst().factor().inCoeffDomain())
      mipoFactors.removeFirst();

#ifdef HAVE_FLINT
    fmpz_poly_clear (v[0]);
    fmpz_poly_clear (v[1]);
    fmpz_poly_clear (w[0]);
    fmpz_poly_clear (w[1]);
    delete [] v;
    delete [] w;
    delete [] link;
    fmpz_poly_factor_clear (liftedFactors);
#endif

    if (mipoFactors.length() > 1 ||
        (mipoFactors.length() == 1 && mipoFactors.getFirst().exp() > 1) ||
         mipo.inCoeffDomain())
    {
        rec=true;
        goto differentevalpoint;
    }
    else
      mipos[i-1]= mipo;
  }

  if (degree (mipos[0]) != degree (mipos[1]))
  {
    rec=true;
    goto differentevalpoint;
  }

  On (SW_RATIONAL);
  if (maxNorm (mipos[0]) < maxNorm (mipos[1]))
    alpha= rootOf (mipos[0]);
  else
    alpha= rootOf (mipos[1]);

  int wrongMipo= 0;

  Variable beta;
  if (maxNorm (mipos[0]) < maxNorm (mipos[1]))
  {
    mipoFactors= factorize (mipos[1], alpha);
    if (mipoFactors.getFirst().factor().inCoeffDomain())
      mipoFactors.removeFirst();
    for (iter= mipoFactors; iter.hasItem(); iter++)
    {
      if (degree (iter.getItem().factor()) > 1)
        wrongMipo++;
    }
    if (wrongMipo == mipoFactors.length())
    {
      rec=true;
      goto differentevalpoint;
    }
    wrongMipo= 0;
    beta= rootOf (mipos[1]);
    mipoFactors= factorize (mipos[0], beta);
    if (mipoFactors.getFirst().factor().inCoeffDomain())
      mipoFactors.removeFirst();
    for (iter= mipoFactors; iter.hasItem(); iter++)
    {
      if (degree (iter.getItem().factor()) > 1)
        wrongMipo++;
    }
    if (wrongMipo == mipoFactors.length())
    {
      rec=true;
      goto differentevalpoint;
    }
  }
  else
  {
    mipoFactors= factorize (mipos[0], alpha);
    if (mipoFactors.getFirst().factor().inCoeffDomain())
      mipoFactors.removeFirst();
    for (iter= mipoFactors; iter.hasItem(); iter++)
    {
      if (degree (iter.getItem().factor()) > 1)
        wrongMipo++;
    }
    if (wrongMipo == mipoFactors.length())
    {
      rec=true;
      goto differentevalpoint;
    }
    wrongMipo= 0;
    beta= rootOf (mipos[0]);
    mipoFactors= factorize (mipos[1], beta);
    if (mipoFactors.getFirst().factor().inCoeffDomain())
      mipoFactors.removeFirst();
    for (iter= mipoFactors; iter.hasItem(); iter++)
    {
      if (degree (iter.getItem().factor()) > 1)
        wrongMipo++;
    }
    if (wrongMipo == mipoFactors.length())
    {
      rec=true;
      goto differentevalpoint;
    }
  }


  CanonicalForm F1;
  if (degree (F,1) > minTdeg)
    F1= F (eval[1], 2);
  else
    F1= F (eval[0], 1);

  CFFList QaF1Factors;
  bool swap= false;
  if (F1.level() == 2)
  {
    swap= true;
    F1=swapvar (F1, x, y);
    F= swapvar (F, x, y);
  }

  wrongMipo= 0;
  QaF1Factors= factorize (F1, alpha);
  if (QaF1Factors.getFirst().factor().inCoeffDomain())
    QaF1Factors.removeFirst();
  for (iter= QaF1Factors; iter.hasItem(); iter++)
  {
    if (degree (iter.getItem().factor()) > minTdeg)
      wrongMipo++;
  }

  if (wrongMipo == QaF1Factors.length())
  {
    rec= true;
    F= bufF;
    goto differentevalpoint;
  }

  CanonicalForm evaluation;
  if (swap)
    evaluation= eval[0];
  else
    evaluation= eval[1];

  F *= bCommonDen (F);
  F= F (y + evaluation, y);

  int liftBound= degree (F,y) + 1;

  modpk b= modpk();

  CanonicalForm den= 1;

  mipo= getMipo (alpha);

  CFList uniFactors;
  for (iter=QaF1Factors; iter.hasItem(); iter++)
    uniFactors.append (iter.getItem().factor());

  F /= Lc (F1);
  ZZX NTLmipo= convertFacCF2NTLZZX (mipo);
  ZZX NTLLcf= convertFacCF2NTLZZX (Lc (F*bCommonDen (F)));
  ZZ NTLf= resultant (NTLmipo, NTLLcf);
  ZZ NTLD= discriminant (NTLmipo);
  den= abs (convertZZ2CF (NTLD*NTLf));

  // make factors elements of Z(a)[x] disable for modularDiophant
  CanonicalForm multiplier= 1;
  for (CFListIterator i= uniFactors; i.hasItem(); i++)
  {
    multiplier *= bCommonDen (i.getItem());
    i.getItem()= i.getItem()*bCommonDen(i.getItem());
  }
  F *= multiplier;
  F *= bCommonDen (F);

  Off (SW_RATIONAL);
  int ii= 0;
  CanonicalForm discMipo= convertZZ2CF (NTLD);
  findGoodPrime (bufF*discMipo,ii);
  findGoodPrime (F1*discMipo,ii);
  findGoodPrime (F*discMipo,ii);

  int pp=cf_getBigPrime(ii);
  b = coeffBound( F, pp, mipo );
  modpk bb= coeffBound (F1, pp, mipo);
  if (bb.getk() > b.getk() ) b=bb;
    bb= coeffBound (F, pp, mipo);
  if (bb.getk() > b.getk() ) b=bb;

  ExtensionInfo dummy= ExtensionInfo (alpha, false);
  DegreePattern degs= DegreePattern (uniFactors);

  bool earlySuccess= false;
  CFList earlyFactors;
  uniFactors= henselLiftAndEarly
              (F, earlySuccess, earlyFactors, degs, liftBound,
               uniFactors, dummy, evaluation, b, den);

  DEBOUTLN (cerr, "lifted factors= " << uniFactors);

  CanonicalForm MODl= power (y, liftBound);

  On (SW_RATIONAL);
  F *= bCommonDen (F);
  Off (SW_RATIONAL);

  CFList biFactors;

  biFactors= factorRecombination (uniFactors, F, MODl, degs, evaluation, 1,
                                  uniFactors.length()/2, b, den);

  On (SW_RATIONAL);

  if (earlySuccess)
    biFactors= Union (earlyFactors, biFactors);
  else if (!earlySuccess && degs.getLength() == 1)
    biFactors= earlyFactors;

  bool swap2= false;
  appendSwapDecompress (biFactors, CFList(), CFList(), swap, swap2, CFMap());
  if (isOn (SW_RATIONAL))
    normalize (biFactors);

  CFAFList result;
  bool found= false;

  for (CFListIterator i= biFactors; i.hasItem(); i++)
  {
    if (full)
      result.append (CFAFactor (i.getItem(), getMipo (alpha), 1));

    if (totaldegree (i.getItem()) == minTdeg)
    {
      if (!full)
        result= CFAFList (CFAFactor (i.getItem(), getMipo (alpha), 1));
      found= true;

      if (!full)
        break;
    }
  }

  if (!found)
  {
    rec= true;
    F= bufF;
    goto differentevalpoint;
  }

  if (isRat)
    On (SW_RATIONAL);
  else
    Off (SW_RATIONAL);

  return result;
}

#endif


