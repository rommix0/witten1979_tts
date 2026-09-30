/*
 *                      ISP global definitions
 *
 *
 * 1  DATA STRUCTURES
 *
 * 1.1  The rule table
 *
 *      This comprises four parts:  the lexical character array
 *      (lex), the lexical index (lexindex), the lex data,
 *      and the miscellaneous parameter table (misc).
 *      There are at most MAXLEX lexes, and at most NRULE sets of
 *      rule data associated with each one.
 */
#define MAXLEX 100
#define NRULE 2
/*
 * 1.1.1  The lexical character array
 */
char lex[MAXLEX][2];
int nlexes;
/*
 *      This contains the lexes as fixed-length 2-character
 *      strings, padded with ' ' if necessary.
 *      The array terminates when lexind = nlexes.
 *      It is searched using int find(ch1, ch2).
 *      The lexes are numbered 0, 1, 2, ...  according to their
 *      position in the array.
 *
 * 1.1.2  The lexical index
 */
int *lexindex[MAXLEX][NRULE];
/*
 *      Given a lex number lexind and a rule table index ruleind,
 *      lexindex[lexind][ruleind] is the address of the
 *      data for that lex in the appropriate set of rules.
 *
 * 1.1.3  The lex data
 *
 *      This is a contiguous block of integers.
 *      Element 0 contains the lex type - there are NLEXTYPS
 *      of these, numbered 0, 1, ..., NLEXTYPS-1.
 */
#define NLEXTYPS 9
/*
 *      The types are
 */
#define EOUTYP  0
#define EOTGTYP 1
#define TGTYP   2
#define FOOTTYP 3
#define WORDTYP 4
#define SYLLTYP 5
#define PHTYP   6
#define DURTYP  7
#define PITTYP  8
/*
 *      In addition, we define a null type (for use
 *      with findlex):
 */
#define NULLTYP -1
/*
 *      For each type, there are nrulelts[lextyp] words of
 *      lex data, so
 *          element 0 is lextyp
 *                  1
 *                  2
 *                 ...
 *              nrulelts[lextyp]-1   is the last word.
 */
int nrulelts[NLEXTYPS];
/*
 *      The lex data which is stored in the rule table is
 *
 *        end-of-utterance      EOUTYP
 *
 *        end-of-tone-group     EOTGTYP
 *
 *        tone group boundary  TGTYP n1 n2 ... n10
 *          [the 10 numbers that define the contour]
 *
 *        foot boundary         FOOTTYP s foot-duration
 *          [s = 0 for unstressed feet, 1 for stressed feet]
 *
 *        word boundary         WORDTYP
 *
 *        syllable boundary     SYLLTYP minlength
 *
 *        phoneme               PHTYP flags mindur av as f1 f2 f3 fh
 *
 *        duration specifier    DURTYP
 *
 *        pitch specifier       PITTYP
 *
 *
 *      The following PHTYP flag bits are defined.
 */
#define VOWEL  000001
#define LVOWEL 000002
#define NASAL  000004
#define BREATH 000010
#define STOP   000020
#define GLIDE  000040
#define H      000100
#define F3X    000200
#define SONOR VOWEL|NASAL|GLIDE
/*
 *        The phoneme data is addressed symbolically.
 */
#define PHFLAGS 1
#define PHDUR   2
#define PHAV    3
#define PHAS    4
#define PHF1    5
#define PHF2    6
#define PHF3    7
#define PHFH    8
/*
 *
 *
 * 1.1.4  The miscellaneous parameter table
 */
int *misc, *ttmisc,
    *aspmisc, *stopmisc, *iclmisc, *iclfric, *syllmisc, *phmisc, *pitmisc;
/*
 *        Misc[0] is the first miscellaneous parameter,
 *        misc[1] the second, and so on.
 *        grule() sets up syllmisc, ..., to point to appropriate
 *        sub-tables within the miscellaneous parameter table.
 *
 *
 * 1.2  The synthesis list
 *
 *      This is a forward linked list.
 *      Each element has a forward pointer and NCONTENT integers.
 */
#define NCONTENT 5
struct listelt {
    struct listelt *fwdptr;
    int content[NCONTENT];
};
/*
 *      There are three types of list elements;
 *      those which refer to lexes, those which do not, and real-time events.
 *
 * 1.2.1  Lex elements
 *
 *        0 <= content[0] < MAXLEX.
 *        Content[0] contains the index of the lex.
 *        Thus lexindex[content[0]][ruleind] gives the address
 *        of the first word of rule data for the lex
 *        (i.e. points to lextyp).
 *
 * 1.2.2  Non-lex elements
 *
 *        MAXLEX < content[0].
 *        Content[0] identifies the type of the element.
 *        Types are
 */
#define SOUTYP  MAXLEX
#define CLUSTYP MAXLEX+1
/*
 * 1.2.3  Events
 *
 *        As with non-lex elements, content[0] identifies the type
 *        of event.  Event types are negative.
 *        Content[1] gives the event time.
 */
#define TARGTYP -1
#define INCRTYP -2
#define ASPTYP  -3
/*
 *      In all cases, content[1], content[2], ... may contain
 *      information pertinent to the particular element.
 *
 *      lex EOUTYP      content[1] - time of eou
 *
 *      lex EOTGTYP     content[1] - time at end of tg
 *
 *      lex FOOTTYP     content[1] - target foot duration
 *                      content[2] - number of syllables in foot
 *
 *      lex SYLLTYP     content[1] - syllable type:
 *                                      0 - strong stressed
 *                                      1 - weak stressed
 *                                      2 - enclitic
 *                                      3 - proclitic
 *                      content[2] - syllable weight
 *                      content[3] - syllable duration
 *
 *      lex PHTYP       content[1] - time at start of this phoneme
 *                      content[2] - total duration
 *                      content[3] - transition-in duration
 *                      content[4] - 0 if phoneme is 'int'
 *                                   1 if phoneme is 'ext'
 *
 *      lex DURTYP      content[1] - total duration of previous
 *                                   phoneme
 *
 *      lex PITTYP      content[1] - time of pitch specification,
 *                                   relative to start of
 *                                   transition of previous
 *                                   phoneme
 *                      content[2] - value of pitch specification
 *
 *      non-lex CLUSTYP content[1] - type of cluster;
 *                                     0 - C1 cluster
 *                                     1 - N cluster
 *                                     2  - C2 cluster
 *                      content[2] - count of segments in cluster
 */
#define C1CLUS 0
#define NUCLUS 1
#define C2CLUS 2

/*
 *      event TARGTYP   content[1] - event time
 *                      content[2] - logical parameter number
 *                                        0  av
 *                                        1  as
 *                                        2  f1
 *                                        3  f2
 *                                        4  f3
 *                                        5  fh
 *                                        6  f0
 *                      content[3] - time-increment
 *                      content[4] - parameter target value
 *
 *      event INCRTYP   content[1] - event time
 *                      content[2] - logical parameter number
 *                      content[3] - time-increment during which
 *                                   event is in force
 *                      content[4] - constant parameter-increment
 *                                   throughout event
 *
 *      event ASPTYP    content[1] - event time
 *                      content[2] -
 *                      content[3] - time-increment during which
 *                                   the speech is aspirated
 *
 *
 * 1.5  Segmental synthesis database
 *
 *      Each synthesizer requires two data-bases,
 *      one for the non-real-time segmental synthesis process,
 *      and the other for the real-time device handler.
 */
struct segdata {
    struct listelt *lastp;      /* points into synthesis list, just before
                                   last phoneme synthesized */
    struct listelt *last;       /* points into synthesis list, at last
                                   phoneme synthesized */
    struct listelt *this;       /* points into synthesis list, at
                                   current phoneme */
    struct listelt *next;       /* points into synthesis list, at next
                                   phoneme to be synthesized */
    int rules;                  /* pointer to the appropriate rule
                                   table */
    int sclock;                 /* contains time up to which synthesis
                                   has been done */
};
/*
 *
 *      NPHYS is number of physical synthesizer parameters to a sample.
 *      NLOG  is number of logical parameters to a sample.
 */
#define NPHYS   8
#define NLOG    7
#define NERRORS 5

struct parmdata {
    int state;              /* set to 0 while synthesizer is inactive,
                                     1 while waiting for interrupt,
                                     2 while computing next lot of parameters,
                                     3 if executing seg from interrupt level,
                                             and waiting for interrupt,
                                     4 if executing seg from interrupt level,
                                             and computing next lot of parameters.
                            isp aborts if interrupt occurs when state<=0 or >=4;
                                increments error[0] if interrupt occurs when
                                     state=2 or 4;
                                 accepts interrupts which occur when state=1 or 3. */
    int parms[NPHYS]; /* buffer of current parameters */
    struct listelt *parmp;    /* points into synthesis list, at current
                                 dequeueing position */
    int clock;              /* clock value for event dequeueing */
    int error[NERRORS];        /* error[0] - incremented whenever interrupt
                                             occurs with state = 2 or 4,
                                  error[1] - incremented whenever an overdue
                                             event is encountered in the
                                             synthesis list,
                                  error[2] - incremented whenever a stray element
                                             is found in the synthesis list,
                                  error[3] - incremented whenever an event
                                             with negative or zero increment-time
                                             is encountered,
                                  error[4] - incremented whenever patscale
                                             cuts off a parameter value */
    int values[NLOG];          /* contains current logical parm values */
    int rates[2][NLOG];        /* contains current rate-of-change of each
                                  logical parm, and the time-increment for
                                  which it is in force */
    int incrs[2][NLOG];        /* contains current temporary increment for
                                  each logical parm, and the time-increment
                                  for which it is in force */
    int asp;                /* contains time-increment for which
                                  aspiration is in force */
};
/*
 *
 *      NSYNS is the number of synthesizers
 */
#define NSYNS 3
struct segdata sd[NSYNS];    /* allocate segdata for each synthesizer */
struct parmdata pd[NSYNS];   /* allocate parmdata for each synthesizer */
int syntyp;     /* current synthesizer type */
#define PATTYP 0
#define ICLTYP 1
#define COMTYP 2
/*
 *
 *      Define duration of "^" at beginning and end of each utterance
 */
#define SOUDUR  3
#define EOUDUR  2
/*
 *      ADVANCLOCK is minimum time by which pseudoclock may lead realclock
 */
#define ADVANCLOCK 25
/*
 *
 *      Interpretation of events.
 *
 *
 *      Parameter target event:
 *
 *          Compute a rate to get from the current parameter value to the
 *          target value in the time-increment.
 *          Starting from the next clock tick, apply this rate at each tick.
 *          Thus if no other events intervene, the target will be
 *          attained at the appropriate time (event-time + time-increment).
 *          When the target is reached, zero the rate so that the
 *          parameter remains constant until another specification.
 *          If another parameter target event for the same parameter
 *          is encountered before this one expires,
 *          all memory of this one is erased.
 *
 *
 *      Parameter increment event:
 *
 *          Starting at the event time, the increment is added
 *          to the current parameter value just prior to output,
 *          up to and including clock tick number
 *          event-time + time-increment - 1.
 *
 *
 *      Asp event:
 *
 *          Starting at the event time, the sound is aspirated,
 *          up to and including clock tick number
 *          event-time + time-increment - 1.
 *
 *
 * 1.3  Space
 *
 *     This is a contiguous area of free space, used for
 *     the lex data, the miscellaneous parameter table, and
 *     the synthesis list.
 *     spacep points to the next free location of space.
 */
#define SPACESIZ 4000
int space[SPACESIZ], *spacep;
/*
/*      The freelist is a linked list of listelts, pointed to by
 *      freep, which have been allocated from space and subsequently
 *      freed.
 */
struct listelt *freep;
/*
 *      It is maintained by struct listelt *alloc(), free(p);
 *
 *
 * 2  MISCELLANEOUS DEFINITIONS
 *
 *     pp is a global pointer to the synlst, used in the prosodic routines.
 *     lpp is the last value of pp (follows along the list, just behind pp).
 *     pr is the rule index used by the prosodic routines.
 */
struct listelt *pp, *lpp;
int pr;
/*
 *       reporterrors says whether to report errors (1) or not (0).
 *       altered by the "@e" directive.
 */
int reporterrors;
/*
 *     omode specifies the output expected from isp.
 *     It can be any of the following.
 */
int omode;
#define OSPEECH 0
#define OSYNLST 1
#define OPARMS  2
/*
 * PAT and ICL device addresses.
 * (COMPUTALKER shares the ICL DRV-11 for the moment).
 */
/* PORT: devices are emulated in port.c (were 0167750, 0167760, 0167760) */
extern struct devregs patdev, icldev;
#define PATADDR (&patdev)
#define ICLADDR (&icldev)
#define COMADDR (&icldev)
/*
 * structure for accessing devices
 */
struct devregs {
    int csr;
    int writebuf;
    int readbuf;
};

#define INTRUPTA 0100
#define CSR1 02
/*
 * channels for output to PDP-11/45
 */
#define PARALLEL 0
#define SERIAL   1
int in_chan;    /*      Serial or parallel input (either 1 or 0)        */
char cha;       /*      One character of context for uttread()          */
/*
 * priorities
 */
#define HIGH 1
#define LOW  0
#define PHIGH 0200
#define PLOW  0
/*
 *
 *
 *
 */
#define ever ;;

#include "port.h"
