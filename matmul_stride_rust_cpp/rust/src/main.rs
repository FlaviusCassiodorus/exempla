use std::time::Instant;

const COLS: usize = 200;
const A_ROWS: usize = 200;
const B_COLS: usize = 200;

fn idxc(i: usize, j: usize, row_stride: usize) -> usize {
    const COL_STRIDE: usize = 1;
    i * row_stride + j * COL_STRIDE
}

fn idxr(i: usize, j: usize, row_stride: usize, col_stride: usize) -> usize {
    i * row_stride + j * col_stride
}

// Both methods use the cache-optimal order ikj

#[inline(never)]
fn comptime(out: &mut [f32], a: &[f32], b: &[f32]) {
    for i in 0..A_ROWS {
        for k in 0..COLS {
            let a_ik = a[idxc(i, k, COLS)];
            for j in 0..B_COLS {
                out[idxc(i, j, B_COLS)] += a_ik * b[idxc(k, j, B_COLS)];
            }
        }
    }
}

#[inline(never)]
fn runtime(
    out: &mut [f32],
    a: &[f32],
    b: &[f32],
    a_col_stride: usize,
    b_col_stride: usize,
    out_col_stride: usize,
) {
    for i in 0..A_ROWS {
        for k in 0..COLS {
            let a_ik = a[idxr(i, k, COLS, a_col_stride)];
            for j in 0..B_COLS {
                out[idxr(i, j, B_COLS, out_col_stride)] +=
                    a_ik * b[idxr(k, j, B_COLS, b_col_stride)];
            }
        }
    }
}

fn main() {
    // Could also use blackbox but reading from input is closer to real usage
    // let stride = std::hint::black_box(1usize);
    let stride: usize = std::env::args()
        .nth(1)
        .expect("Missing stride argument")
        .parse()
        .expect("Stride must be a number");

    let a: Vec<f32> = (0..A_ROWS * COLS).map(|i| i as f32 * 0.1).collect();
    let b: Vec<f32> = (0..COLS * B_COLS).map(|i| i as f32 * 0.2).collect();
    let mut outc = vec![0.0; A_ROWS * B_COLS];
    let mut outr = vec![0.0; A_ROWS * B_COLS];

    // We could do more samples and switch the order
    // but I am keeping it simple
    // Letting runtime run last

    // Do some warm up runs
    for _ in 0..10 {
        comptime(&mut outc, &a, &b);
    }
    let tic = Instant::now();
    comptime(&mut outc, &a, &b);
    let elapsed_c = (Instant::now() - tic).as_micros() as f32 / 1000.0;

    for _ in 0..10 {
        runtime(&mut outr, &a, &b, stride, stride, stride);
    }
    let tic = Instant::now();
    runtime(&mut outr, &a, &b, stride, stride, stride);
    let elapsed_r = (Instant::now() - tic).as_micros() as f32 / 1000.0;

    for (x, y) in outc.iter().zip(&outr) {
        assert_eq!(*x, *y);
    }

    println!("Stride=1 at compile time: {:.2} ms", elapsed_c);
    println!("Stride={} at runtime: {:.2} ms", stride, elapsed_r);
    let ratio = elapsed_r / elapsed_c;
    println!("Ratio: {:.1}x times slower", ratio);
}
